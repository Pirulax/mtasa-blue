#include <StdInc.h>
#include "CValues.h"
#include <lua/CLuaStackChecker.h>

static auto GetGame()
{
#ifdef MTA_CLIENT
    return g_pClientGame;
#else
    return g_pGame;
#endif
}

static auto GetScriptDebugging()
{
    return GetGame()->GetScriptDebugging();
}

namespace lua
{
/* Lua Read/Write */

/*  reads `count` values starting at `idxbegin` towards the stack top.
 * `idxbegin`: regular Lua index (might be negative)
 * 'count': # of values to read. -1 to read all values in range `idxbegin` - stack top (inclusive).
 */
void CValues::Read(lua_State* L, int idxbegin, int count)
{
    if (idxbegin < 0)
        idxbegin = lua_gettop(L) - idxbegin + 1; /* negative to positive index */

    if (lua_type(L, idxbegin) == LUA_TNONE) /* nothing to read */
        throw std::logic_error{ "Nothing to read from Lua stack" };

    if (count == -1)
        count = lua_gettop(L) - idxbegin + 1; /* read all beginning at `idxbegin` */

    for (int idx = idxbegin; idx < idxbegin + count; idx++)
    {
        dassert(lua_type(L, idx) != LUA_TNONE);
        m_values.emplace_back(L, idx);
    }
}

void CValues::WriteAsTable(lua_State* L) const
{
    lua_createtable(L, m_values.size(), 0);
    LUA_STACK_EXPECT(0);

    int i = 1;
    for (const auto& v : m_values)
    {
        v.Write(L);
        lua_rawseti(L, -2, i++);
    }   
}

void CValues::Write(lua_State* L) const
{
    for (const auto& v : m_values)
        v.Write(L);
}

/* Push methods for different types */
#ifdef MTA_CLIENT
CValue& CValues::Push(CClientEntity* value)
#else
CValue& CValues::Push(CElement* value)
#endif
{
    return Push(static_cast<CValue::UserData>(value->GetID().Value()));
}

CValue& CValues::Push(CLuaTimer* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CResource* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

#ifndef MTA_CLIENT
CValue& CValues::Push(CDbJobData* value)
{
    return Push(static_cast<CValue::UserData>(value->GetId()));
}

CValue& CValues::Push(CTextItem* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CTextDisplay* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CAccount* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CAccessControlListGroup* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CAccessControlList* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}

CValue& CValues::Push(CBan* value)
{
    return Push(static_cast<CValue::UserData>(value->GetScriptID()));
}
#endif

template<typename Fn, typename TimePoint>
auto UpdateLuaFnTiming(CLuaMain* lmain, Fn&& fn, TimePoint beginTp)
{
    using namespace std::chrono;
#ifdef MTA_CLIENT
    using CPerfStatLuaTiming = CClientPerfStatLuaTiming;
#endif
    CPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(
        lmain,
        std::forward<Fn>(fn),
        static_cast<TIMEUS>(duration_cast<microseconds>(TimePoint::clock::now() - beginTp).count())
    );
}

/* TODO: Move this call stuff to CLuaFunctionRef or something */

/* call the function on top of the stack. Caller should clean the Lua stack */
bool CValues::CallFunctionOnStack(CLuaMain* lmain, CValues* outReturnedValues) const
{
    lua_State* const L = lmain->GetVirtualMachine();
    dassert(lua_type(L, -1) == LUA_TFUNCTION);

    const auto originalTop = lua_gettop(L) - 1; /* -1 as compensation for the function on the stack */
    Write(L); /* push our values to L's stack */
    lmain->ResetInstructionCount();

    switch (lmain->PCall(L, m_values.size(), LUA_MULTRET, 0))
    {
    case LUA_ERRRUN:
    case LUA_ERRMEM:
    {
        GetScriptDebugging()->LogPCallError(L, ConformResourcePath(lua_tostring(L, -1)));
        return false;
    }
    default:
    {
        if (outReturnedValues)
            outReturnedValues->Read(L, originalTop, -1); /* store all returned values */
        return true;
    }
    }
}

/* Code simplification */
template<typename Fn>
struct Guard
{
    Guard(Fn&& fn) : m_fn(std::forward<Fn>(fn)) {}
    ~Guard() { m_fn(); }
    Fn m_fn;
};

bool CValues::Call(CLuaMain* lmain, const CLuaFunctionRef& fn, CValues* outReturnedValues) const
{
    lua_State* const L = lmain->GetVirtualMachine();
    LUA_CHECKSTACK(L, 1);

    /* does timing and stack cleanup on function return */
    Guard guard{[=, &fn, begin = std::chrono::high_resolution_clock::now(), top = lua_gettop(L)] {
        UpdateLuaFnTiming(lmain, lmain->GetFunctionTag(fn.ToInt()), begin);
        lua_settop(L, top);
    }};

    lua_getref(L, fn.ToInt()); /* push function on the stack */
    return CallFunctionOnStack(lmain, outReturnedValues);
}

bool CValues::CallGlobal(CLuaMain* lmain, const char* fn, CValues* outReturnedValues) const
{
    assert(fn);

    lua_State* const L = lmain->GetVirtualMachine();
    LUA_CHECKSTACK(L, 1);

    /* does timing and stack cleanup on function return */
    Guard guard{ [=, begin = std::chrono::high_resolution_clock::now(), top = lua_gettop(L)] {
        UpdateLuaFnTiming(lmain, fn, begin);
        lua_settop(L, top);
    }};

    lua_pushstring(L, fn);
    lua_rawget(L, LUA_GLOBALSINDEX); /* pusn fn on stack */

    if (lua_isfunction(L, -1))
        return CallFunctionOnStack(lmain, outReturnedValues);
    return false;
}

/* BitStream Read/Write */
void CValues::Read(NetBitStreamInterface& bitStream)
{
    uint32_t nargs;
    if (!bitStream.ReadCompressed(nargs))
        throw std::runtime_error{ "Protocol error" };

    CValue::ReferencedTables tables;
    tables.emplace_back(); /* Emplace empty ref as placeholder for backwards compatibility */

    for (size_t i = 0; i < nargs; i++)
    {
        /* Have to construct it this was, because vector can't access the constructor */
        /* It will be move constructed anyways, so no worries */
        m_values.push_back({ bitStream, tables });
    }
}

void CValues::Write(NetBitStreamInterface& bitStream) const
{
    size_t nextRef = 1; /* (start from 1) for backwards compatibility*/
    bitStream.WriteCompressed(static_cast<uint32_t>(m_values.size()));
    for (const auto& v : m_values)
        v.Write(bitStream, nextRef);
}

/* Wrap a JSON object into a unique ptr for automatic freeing */
auto WrapJSONObject(json_object* object)
{
    const auto Deleter = [](json_object* obj) { json_object_put(obj); };
    return std::unique_ptr<json_object, decltype(Deleter)>{ object, Deleter };
}

/* JSON Read/Write */
void CValues::Read(const char* json)
{
    /* Fast JSON check: Check first non-white space character is '[' or '{' */
    for (const char* ptr = json; *ptr; ptr++)
    {
        auto c = static_cast<unsigned char>(*ptr); /* must use uchar */
        if (c == '[' || c == '{')
            break;
        if (!isspace(c))
            throw std::runtime_error{ "JSON string most start with <space>, '[' or '{'" };
    }

    /* unique_ptr with custom deleter, so nobody forgets freeing */

    json_tokener_error jerror;
    auto jobj = WrapJSONObject(json_tokener_parse_verbose(json, &jerror));
    if (!jobj)
        throw std::runtime_error{ std::string("Failed to parse JSON. Parser says: ") + json_tokener_error_desc(jerror) };

    switch (json_object_get_type(jobj.get())) 
    {
    case json_type_array:
    {
        
        size_t len = json_object_array_length(jobj.get());
        for (size_t i = 0; i < len; i++)
            m_values.emplace_back(json_object_array_get_idx(jobj.get(), i));
        break;
    }
    case json_type_object:
    {
        m_values.emplace_back(jobj.get());
        break;
    }
    default:
        throw std::runtime_error{ "Failed to read JSON. Top-most value should be an array or object." };
    }
}

void CValues::Write(std::string& json, bool serialize, int flags)
{
    auto jarray = WrapJSONObject(json_object_new_array());
    if (!jarray)
        throw std::runtime_error{ "Failed to create array (probably out of memory)" };

    size_t nextRef = 1; // Start at 1 as per old behaviour `this` is counted as a variable as well
    for (const auto& value : m_values)
    {
        if (json_object* vobj = value.Write(serialize))
            json_object_array_add(jarray.get(), vobj);
        else
            throw std::runtime_error{ "Failed to serialize value" };
    }

    size_t length;
    const char* str = json_object_to_json_string_length(jarray.get(), flags, &length);
    json.assign(str, length); /* might throw (Please don't get rid of `str` as argument eval order isn't guaranteed)*/
    /* the wrapped json object is freed automatically */
}
};
