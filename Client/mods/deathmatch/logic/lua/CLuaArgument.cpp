/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/lua/CLuaArgument.cpp
 *  PURPOSE:     Lua argument class
 *
 *****************************************************************************/

#include "StdInc.h"
#include "net/SyncStructures.h"
#include "CResource.h"
#include <lua/LuaBasic.h>
#include <lua/CLuaStackChecker.h>
#include <charconv>
#include <logic/CClientPerfStatModule.h>

#ifdef snprintf
#undef snprintf
#endif

#define ARGUMENT_TYPE_INT       9
#define ARGUMENT_TYPE_FLOAT     10

#ifndef VERIFY_ENTITY
#define VERIFY_ENTITY(entity) (CStaticFunctionDefinitions::GetRootElement()->IsMyChild(entity,true)&&!entity->IsBeingDeleted())
#endif

#ifndef VERIFY_RESOURCE
#define VERIFY_RESOURCE(resource) (g_pClientGame->GetResourceManager()->Exists(resource))
#endif

bool IsSpecialStringInJSON(std::string_view str)
{
    return str.length() > 3 && str[0] == '^' && str[1] != '^' && str[2] == '^';
}

auto GetGame()
{
#ifdef MTA_CLIENT
    return g_pClientGame;
#else
    return g_pGame;
#endif
}

auto GetScriptDebugging()
{
    return GetGame()->GetScriptDebugging();
}

void LogUnableToPacketize(const char* szMessage)
{
#ifdef MTA_DEBU /* todo */
    if (m_strFilename.length() > 0)
        GetScriptDebugging()->LogWarning(NULL, "%s:%d: %s\n", ConformResourcePath(m_strFilename.c_str()).c_str(), m_iLine, szMessage);
    else
#endif
        GetScriptDebugging()->LogWarning(NULL, "Unknown: %s\n", szMessage);
}

namespace lua
{
void CValue::Read(lua_State* L, int idx, LuaCopiedValuesMap& copiedTables)
{
    switch (lua_type(L, idx))
    {
    case LUA_TNUMBER:
        m_value.emplace<Number>(lua_tonumber(L, idx));
        break;
    case LUA_TSTRING:
        size_t len;
        const char* value = lua_tolstring(L, idx, &len);
        m_value.emplace<String>(value, len);
        break;
    case LUA_TBOOLEAN:
        m_value.emplace<Bool>(lua_toboolean(L, idx));
        break;
    case LUA_TUSERDATA:
        m_value.emplace<UserData>(*static_cast<UserData*>(lua_touserdata(L, idx)));
        break;
    case LUA_TLIGHTUSERDATA:
        m_value.emplace<UserData>(reinterpret_cast<UserData>(lua_touserdata(L, idx)));
        break;
    case LUA_TTABLE:
    {
        
        if (const auto it = copiedTables.find(lua_topointer(L, idx)); it == copiedTables.end()) /* copied yet? */
        {
            /* no, so copy and store it */
            LUA_CHECKSTACK(L, 2);

            lua_pushnil(L); /* first key */
            if (idx < 0) /* correct the table index (because of the above push) */
                idx--;

            /* TODO: Preallocate table in a sensible way.. Maybe iter thru all values? */
            Table& table = m_value.emplace<Table>();
            // table->SetArrKeysCount(lua_objlen(L, -1)); TODO Somehow make this work... 

            copiedTables[lua_topointer(L, idx)] = table; /* Mark as copied before copying others */
            while (lua_next(L, idx))
            {
                /* emplace 2 values: key (at index -2) & value at index -1) */
                table->push_back({ {L, -2, copiedTables}, {L, -1, copiedTables} });

                /* removes 'value'; keeps 'key' for next iteration */
                lua_pop(L, 1);
            }
        }
        else
        {
            /* copied already, just store a ref to it */
            m_value.emplace<TableRef>(it->second);
        }

        break;
    }
    case LUA_TNIL:
        m_value.emplace<Nil>();
        break;
    case LUA_TNONE:
    case LUA_TTHREAD:   // TODO - probably not possible
    case LUA_TFUNCTION: // TODO - has to work inside tables too
    default:
        m_value.emplace<None>();
        break;
    }
}

void CValue::Write(lua_State* L, LuaRefList& refs) const
{
    std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        LUA_CHECKSTACK(L, 1);

        if constexpr (std::is_same_v<T, Table>)
        {        
            /*
            * TL;DR; Calculate the array and hash part size for Lua, this way
            * we can reduce the number of allocations.
            * 
            * Okay, this is rough estimation of the size of the array part in Lua.
            * Because of the way luaH_next works (ltable.c) at first it returns
            * elements from the array part, thus their key can only be numeric.
            * 
            * Now, read this from the Lua implementation stuff(https://www.lua.org/doc/jucs05.pdf):
            * The computed size of the array part
            * is the largest n such that at least half the slots between 1 and n are in use
            * (to avoid wasting space with sparse arrays) and there is at least one used slot
            * between n/2 + 1 and n (to avoid a size n when n/2 would do).
            *
            * And you will roughly understand what's going on here.
            *
            * If this calculation is incorrect, that isn't a problem, but we dont
            * want to overallocate by a lot, since that wastes memory.
            * 
            * 
            */      
            //Number last = 0, empty = 0; /* last must be 0, otherwise `emptyBetween` will be -1 at first iter */
            //size_t i;
            //for (size_t end = value.size(); i < end; i++)
            //{
            //    const bool stop = std::visit([&](const auto& k) mutable {
            //        /* `n` (in the docs above) is `k` here */
            //        if constexpr (std::is_same_v<std::decay_t<decltype(k)>, Number>)
            //        {
            //            if (std::trunc(k) != k) /* check is int */
            //                return true;

            //            /* if equal with last key, we're in the hash part already.
            //             * this also handles < 0 check, since is last is always >= 0 */
            //            if (k <= last)
            //                return true;

            //            empty += k - last - 1;
            //            if (empty / 2 > k)
            //                return true; /* more than half of the slots are empty */

            //            if (last < k / 2 + 1)
            //                return true; /* no used slot between k / 2 + 1 and k */

            //            last = k;

            //            return false;
            //        }
            //        return true; /* non-numeric key found */
            //    }, value[i].first.m_value);
            //    if (stop)
            //        break;
            //}

            ///* `last` is the highest key in the array part. Add 1 for safety */
            //const int narr = static_cast<int>(last + 1);
            //const int nrec = static_cast<int>(value->size() - i + 1);
            lua_createtable(L, value->GetArrKeysCount(), value->GetRecKeysCount());
            value->SetRef(refs.Add());

            for (const auto& [k, v] : *value)
            {
                k.Write(L, refs);
                v.Write(L, refs);
                lua_rawset(L, -3);
            }
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            refs.Get(value->GetRef());
            dassert(lua_type(L, -1) == LUA_TTABLE);
        }
        else if constexpr (!std::is_same_v<T, None>) /* can't push None */
        {
            lua::Push(L, value);
        }
    }, m_value);
}

bool CValue::Read(NetBitStreamInterface& bitStream, TableList& tables)
{
    SLuaTypeSync type;
    if (!bitStream.Read(&type))
        return false;

    switch (type.data.ucType)
    {
    case LUA_TNUMBER:
    {
        if (bitStream.ReadBit())
        {
            if (bitStream.ReadBit())
            {
                if (double v; bitStream.Read(v))
                    m_value.emplace<Number>(v);
                else
                    return false;
            }
            else
            {
                if (float v; bitStream.Read(v))
                    m_value.emplace<Number>(RoundFromFloatSource(v));
                else
                    return false;
            }
        }
        else
        {
            if (int v; bitStream.ReadCompressed(v))
                m_value.emplace<Number>(v);
            else
                return false;
        }
        break;
    }
    case LUA_TSTRING:
    {
        String& value = m_value.emplace<String>();
        if (unsigned short length; bitStream.ReadCompressed(length) && length)
        {
            if (!bitStream.CanReadNumberOfBytes(length))
                return false;

            value.resize(length);
            bitStream.Read(value.data(), length); /* Read data directly into `value` */
        }
        break;
    }
    case LUA_TSTRING_LONG:
    {
        // Nearly identical to above, but with `uint` length header + aligned read
        String& value = m_value.emplace<String>();
        if (unsigned int length; bitStream.ReadCompressed(length) && length)
        {
            if (!bitStream.CanReadNumberOfBytes(length))
                return false;

            bitStream.AlignReadToByteBoundary();

            value.resize(length);
            bitStream.Read(value.data(), length); /* Read data directly into `value` */
        }
        break;
    }
    case LUA_TBOOLEAN:
    {
        if (bool v; bitStream.ReadBit(v))
            m_value.emplace<Bool>(v);
        break;
    }
    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
    {
        if (ElementID v; bitStream.Read(v))
            m_value.emplace<UserData>(v.Value());
        break;
    }
    case LUA_TTABLE:
    {
        Table& table = m_value.emplace<Table>();

     
        if (unsigned int nvals; bitStream.ReadCompressed(nvals))
        {
            Table& table = m_value.emplace<Table>();
            table->reserve(nvals);
            tables.emplace_back(table);
            dassert(nvals % 2 == 0); /* number of all elements must be even because a pair is 2 values */
            for (size_t i = 0; i < nvals; i += 2) /* originally it read k and v in separate iterations, thus i += 2 since we read both / iteration */
                table->push_back({ { bitStream, tables }, { bitStream, tables } }); /* Read k and v at the same time */
        }


        /* TODO: ValidateTableKeys() */

        break;
    }
    case LUA_TTABLEREF:
    {
        /*
         * Wargning: If we were to use `Read` this might fail as
         * type sizes may not be the same across compilers.
         *Eg.: gcc x64(server) vs msvc x86 (client)
         */
        if (unsigned long refid; bitStream.ReadCompressed(refid))
        {
            if (refid >= tables.size())
                return false;
            m_value.emplace<TableRef>(tables[refid]);
        }
        else
            return false;
        break;
    }
    case LUA_TNIL:
        m_value.emplace<Nil>();
        break;
    case LUA_TNONE:
    default:
        m_value.emplace<None>();
        break;
    }
}

bool CValue::Write(NetBitStreamInterface& bitStream, size_t& lastTableRef) const
{
    return std::visit([&, this](auto& value) -> bool {
        using T = std::decay_t<decltype(value)>;

       const auto WriteType = [&bitStream](auto type) {
            SLuaTypeSync sync{ type };
            sync.Write(bitStream);
        };

        if constexpr (std::is_same_v<T, Number>)
        {
            WriteType(LUA_TNUMBER);
            CompressArithmetic(value, [&bitStream, this](auto compressedValue) {
                using CT = decltype(compressedValue);
                if constexpr (std::is_same_v<CT, int>)
                {
                    bitStream.WriteBit(false); /* is floating point  */
                    bitStream.WriteCompressed(compressedValue);
                }
                else constexpr (std::is_floating_point_v<CT>)
                {
                    bitStream.WriteBit(true); /* is floating point */
                    bitStream.WriteBit(std::is_same_v<CT, double>); /* is double */
                    bitStream.Write(compressedValue);
                }
            });
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            const auto WriteString = [&](auto length, auto type) {
                WriteType(type);
                bitStream.WriteCompressed(length);
                if (length)
                {
                    if (type == LUA_TSTRING_LONG)
                        bitStream.AlignWriteToByteBoundary();
                    bitStream.Write(value.data(), length);
                }
            };

            if (std::numeric_limits<unsigned short>::max() >= value.length()) /* length fits in a ushort? */
                WriteString(static_cast<unsigned short>(value.length()), LUA_TSTRING);
            else
                WriteString(static_cast<unsigned int>(value.length()), LUA_TSTRING_LONG); /* no, use unsigned int */
        }
        else if constexpr (std::is_same_v<T, Bool>)
        {
            WriteType(LUA_TBOOLEAN);
            bitStream.WriteBit(value);
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            ElementID id{ value };
            if (CElementIDs::GetElement(id)) /* check if exists */
            {
                WriteType(LUA_TUSERDATA);
                bitStream.Write(id);
            }
            else /* might happen if userdata is refernced in lua, but element is destroyed? */
                WriteType(LUA_TNIL); /* write a nil though so other side won't get out of sync */
        }
        else if constexpr (std::is_same_v<T, Table>)
        {
            
            WriteType(LUA_TTABLE);
            bitStream.WriteCompressed(static_cast<unsigned int>(value->size() * 2)); /* write the number of all values (as per old behaviour) */
            //if (bitStream.Can(eBitStreamVersion::CValueNArr)) /* todo: add to read as well. todo: actually implement this */
                //bitStream.WriteCompressed(value->GetArrKeysCount());
            value->SetRef(lastTableRef++);

            for (const auto& [k, v] : *value)
            {
                k.Write(bitStream, lastTableRef);
                v.Write(bitStream, lastTableRef);
            }
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            WriteType(LUA_TTABLEREF);
            bitStream.WriteCompressed(value->GetRef()); /* write table ref */
        }
        else if constexpr (std::is_same_v<T, Nil>)
        {
            WriteType(LUA_TNIL);
        }
        else
        {
            LogUnableToPacketize("Couldn't packetize argument list, unknown type specified.");
            WriteType(LUA_TNIL); /* write a nil though so other side won't get out of sync */
            dassert(0);
        }
        return true;
    }, m_value);
}

bool CValue::Read(json_object* jobj, TableList& tables)
{
    switch (json_object_get_type(jobj))
    {
    case json_type_double:
    case json_type_int:
    {
        m_value.emplace<Number>(json_object_get_double(jobj));
        break;
    }
    case json_type_string:
    {
        std::string_view value{
            json_object_get_string(jobj),
            json_object_get_string_len(jobj)
        };
        if (!IsSpecialStringInJSON(value))
        {
            m_value.emplace<String>(value);
        }
        else /* uncommon case */
        {
            /* read number after special identifiers (first 3 chars) */
            const auto ReadNumber = [value](auto& out) {
                return std::from_chars(value.data() + 3, value.data() + value.length(), out);
            };

            switch (value[1])
            {
            case 'T': /* table ref */
            {
                size_t ref;
                if (auto [p, c] = ReadNumber(ref); c == std::errc())
                {
                    try
                    {
                        m_value.emplace<TableRef>(tables.at(ref));
                    }
                    catch (std::out_of_range)
                    {
                        GetScriptDebugging()->LogError(nullptr,
                            "Invalid table reference specified in JSON string '%.*s'.", value.length(), value.data());
                        m_value.emplace<Nil>();
                    }
                }
                else
                    dassert(0);
            }
            case 'R': /* resource */
            {
                if (CResource* res = GetGame()->GetResourceManager()->GetResource(value))
                    m_value.emplace<UserData>(res->GetScriptID());
                else
                {
                    GetScriptDebugging()->LogError(NULL,
                        "Invalid resource specified in JSON string '%.*s'.", value.length(), value.data());
                }
                break;
            }
            case 'E': /* element */
            {
                size_t id;
                const auto begin = value.data();
                if (auto [p, c] = ReadNumber(id); c == std::errc())
                {
                    if (CElementIDs::GetElement(id)) /* check is element valid */
                        m_value.emplace<UserData>(id);
                    else
                        m_value.emplace<Nil>(); /* Appears sometimes when a player quits */
                }
                else
                {
                    dassert(0);
                }
                break;

                break;
            }
            }
        }
        break;
    }
    case json_type_boolean:
    {
        m_value.emplace<Bool>(json_object_get_boolean(jobj) == TRUE);
        break;
    }
    case json_type_array:
    {
        Table& table = m_value.emplace<Table>();
        tables.emplace_back(table);
        size_t len = json_object_array_length(jobj);
        table->reserve(len);
        for (size_t i = 0; i < len; i++)
        {
            table->push_back({
                { static_cast<Number>(i) }, /* key */
                { json_object_array_get_idx(jobj, i), tables } /* value */
            });
        }
        break;
    }
    case json_type_object:
    {
        Table& table = m_value.emplace<Table>();
        tables.emplace_back(&table);
        table->reserve(json_object_object_length(jobj));
        json_object_object_foreach(jobj, k, v)
        {
            table->push_back({ { k }, { v, tables }});
        }
        break;
    }
    case json_type_null:
    {
        m_value.emplace<Nil>();
        break;
    }
    }

}

json_object* CValue::Write(bool serialize, size_t& nextRef) const
{
    return std::visit([&, this](const auto& value) -> json_object* {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, Number>)
        {
            int ivalue;
            if (ShouldUseInt(value, &ivalue))
                return json_object_new_int(ivalue);
            else
                return json_object_new_double(value);
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            const auto DoWrite = [](const auto& str) -> json_object* {
                if (str.length() <= std::numeric_limits<unsigned short>::max())
                    return json_object_new_string_len(str.c_str(), str.length());
                else
                    GetScriptDebugging()->LogError(
                        nullptr, "Couldn't convert argument list to JSON. Invalid string specified, limit is 65535 characters.");
                return nullptr;
            };
       
            if (!IsSpecialStringInJSON(value))
                return DoWrite(value);
            else
            {
                /* Prevent clash with how MTA stores elements, resources and table refs as strings.
                 * Sadly we're buried under `const` so we can't modify the original value
                 * This is a rare case scenario anyways. */
                std::string modifiableValue{ value };
                modifiableValue[2] = '~';
                return DoWrite(modifiableValue);
            }

        }
        else if constexpr (std::is_same_v<T, Bool>)
        {
            return json_object_new_boolean(value);
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            if (CResource* res = GetGame()->GetResourceManager()->GetResourceFromScriptID(static_cast<unsigned int>(value)))
            {
                char buffer[2 * MAX_RESOURCE_NAME_LENGTH];
            #ifdef MTA_CLIENT
                std::snprintf(buffer, sizeof(buffer), "^R^%s", res->GetName());
            #else
                std::snprintf(buffer, sizeof(buffer), "^R^%s", res->GetName().c_str());
            #endif
                return json_object_new_string(buffer);
            }
            else if (CElementIDs::GetElement(value))
            {
                if (serialize) /* Elements are dynamic, so storing them is potentially unsafe */
                {
                    char buffer[128];
                    std::snprintf(buffer, sizeof(buffer), "^E^%d", static_cast<unsigned int>(value));
                    return json_object_new_string(buffer);
                }
                else
                {
                    GetScriptDebugging()->LogError(nullptr,
                        "Couldn't convert userdata argument to JSON, elements not allowed for this function.");
                }
            }
            else
            {
                if (serialize)
                {
                    GetScriptDebugging()->LogError(nullptr,
                        "Couldn't convert userdata argument to JSON, only valid elements or resources can be included.");
                }
                else
                {
                    GetScriptDebugging()->LogError(nullptr,
                        "Couldn't convert userdata argument to JSON, only valid resources can be included for this function.");
                }
            }
            return nullptr;
        }
        else if constexpr (std::is_same_v<T, Table>)
        {
            /* check if consequent keys are exactly 1 apart */
            const auto invalidArrayKeyIt = std::find_if(value->begin(), value->end(), [expected = Number(0)](const auto& kv) mutable {
                std::visit([&, this](auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Number>)
                    {
                        if (last + 1 != value) /* there should be exactly 1 between the two indices */
                            return true;
                        last++;
                        dassert(std::trunc(value) != value); /* check is it really numeric, if not we fucked up something */
                        return false;
                    }
                    return true; /* non-numeric key */
                }, kv.first);
            });

            /* insert before write */
            value->SetRef(nextRef++);

            if (invalidArrayKeyIt == value->end())
            {
                /* amazing, we can use an array! Yay! */
                json_object* jarray = json_object_new_array();
                for (const auto& [k, v] : *value)
                {
                    if (json_object* value = v.Write(serialize, nextRef))
                        json_object_array_add(jarray, value);
                    else
                        break;
                }
                return jarray;
            }
            else
            {
                /* must use an object :( */
                json_object* jobject = json_object_new_object();
                for (const auto& [k, v] : *value)
                {
                    char keyBuffer[255]; /* TODO: Use a bigger buffer here / use somehow allocate buffer in jsonc */
                    keyBuffer[0] = 0;
                    if (!k.Write(keyBuffer, sizeof(keyBuffer)))
                        break;

                    if (json_object* value = v.Write(serialize, nextRef))
                        json_object_array_add(jobject, value);
                    else
                        break;
                }
            }
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            char szTableID[64];
            std::snprintf(szTableID, sizeof(szTableID), "^T^%lu", static_cast<long unsigned>(value->GetRef()));
            return json_object_new_string(szTableID);
        }
        else if constexpr (std::is_same_v<T, Nil>)
        {
            return json_object_new_int(0);
        }
        else
        {
            GetScriptDebugging()->LogError(nullptr, 
                "Couldn't convert argument list to JSON, unsupported data type. Use Table, Nil, String, Number, Boolean, Resource or Element.");
        }
            return nullptr;
    }, m_value);
}

/* TODO: Refactor somehow to C++ style.. */
char* CValue::Write(char* buffer, size_t buffsize) const
{
    return std::visit([&, this](const auto& value) -> char* {
        using T = std::decay_t<decltype(value)>;

        /* Can't be used as it generated compiler warning incorrectly..
        const auto PrintToBuffer = [=](auto... values) {
            std::snprintf(buffer, buffsize, values...);
        };*/

        if constexpr (std::is_same_v<T, Number>)
        {
            int ivalue;
            if (ShouldUseInt(value, &ivalue))
                std::snprintf(buffer, buffsize, "%d", ivalue);
            else
                std::snprintf(buffer, buffsize, "%f", value);
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            if (value.length() <= std::numeric_limits<unsigned short>::max()) /* As per old behaviour */
            {
                if (value.length() > buffsize - 1) /* 1 for null terminator */
                {
                    GetScriptDebugging()->LogWarning(
                        nullptr, "Truncating JSOM key to %u as it's too long (%u characters).", (unsigned)buffsize, (unsigned)value.length());
                }
                /* DON'T FORGET TO USE %s AS FORMAT HERE! */
                std::snprintf(buffer, buffsize, "%s", value.c_str()); /* TODO: return c_str() here, and use that without printing to buffer */
            }
            else
            {
                GetScriptDebugging()->LogError(
                    nullptr, "String is too long. Limit is %u characters.", (unsigned)buffsize);
                return nullptr;
            }
        }
        else if constexpr (std::is_same_v<T, Bool>)
        {
            std::snprintf(buffer, buffsize, "%s", value ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            if (CElementIDs::GetElement({ value }))
            {
                std::snprintf(buffer, buffsize, "#E#%d", (int)value);
            }
            else if (CResource* res = GetGame()->GetResourceManager()->GetResourceFromScriptID(value))
            {
            #ifdef MTA_CLIENT
                std::snprintf(buffer, buffsize, "#R#%s", res->GetName());
            #else
                std::snprintf(buffer, buffsize, "#R#%s", res->GetName().c_str());
            #endif // MTA_CLIENT
            }
            else
            {
                GetScriptDebugging()->LogError(
                    nullptr, "Couldn't convert element to string, only valid elements can be sent.");
                return nullptr;
            }
        }
        else if constexpr (std::is_same_v<T, Nil>)
        {
            std::snprintf(buffer, buffsize, "0");
        }
        else if constexpr (std::is_same_v<T, Table> || std::is_same_v<T, TableRef>)
        {
            GetScriptDebugging()->LogError(
                nullptr, "Cannot convert table to string (do not use tables as keys in tables if you want to send them over http/JSON).");
            return nullptr;
        }
        else
        {
            GetScriptDebugging()->LogError(
                nullptr, "Couldn't convert argument to string, unsupported data type. Use String, Number, Boolean or Element.");
            return nullptr;
        }
        return buffer;
    }, m_value);
}

/* TODO: Make table comparasions work. See note there. */
bool CValue::EqualTo(const CValue& other, ComparedTablesSet* compared) const
{
    if (other.m_value.index() != m_value.index())
        return false;

    // Can't use variant::operator== as we need to pass in `comparedValues`
    return std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        const T& otherValue = std::get<T>(other.m_value);
        if constexpr (std::is_same_v<T, Table>)
        {
            return false;

            /* We're the table owner, so this table can't possibly exist */
            //dassert(compared->find(&value) == compared->end());
          
            //if (value.size() != otherValue.size())
            //    return false;

            //compared->insert(&value);

            ///* TODO: Theory: This might not be correct:
            //* I'm not sure about Lua's table implementation, but:
            //* Consider:
            //* t1 = {key2 = true, key1 = true}
            //* t2 = {key1 = true, key2 = true}
            //* Now, since we're using `vector`s we're comparing them step by step
            //* which means in our case t1 != t2 might not be equal, but they should be.
            //* Now, Lua might actually repesent these tables in the same layout in memory
            //* so when we iterate thru them, they might be the same.
            //*/
            // Lua docs(https://www.lua.org/doc/jucs05.pdf):
            // The hash part uses a mix of chained scatter table with Brent’s variation [3].
            // A main invariant of these tables is that if an element is not in its main position
            // (i.e., the original position given by its hash value), then the colliding element
            // is in its own main position.In other words, there are collisions only when two
            // elements have the same main position(i.e., the same hash values for that table
            // size).There are no secondary collisions.Because of that, the load factor of these
            // tables can be 100 % without performance penalties
            //
            // This means 
            //
            // 
            //auto it = otherValue.begin();
            //for (const auto& [k, v] : value)
            //{
            //    if (!k.EqualTo(it->first, compared))
            //        return false;
            //    if (!v.EqualTo(it->second, compared))
            //        return false;
            //    it++;
            //}
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            /* TableRef must refer to an already tested table */
            //return compared->find(value) != compared->end();
            return false;
        }
        else if constexpr (std::is_same_v<T, None> || std::is_same_v<T, Nil>) /* These two are always equal */
        {
            return true;
        }
        else
        {
            return otherValue == value;
        }
    }, m_value);
}

/* Lua Read/Write */

/*  reads `count` values starting at `idxbegin`.
 * `idxbegin` is a regular Lua index (might be negative)
 * pass in -1 as `count` to read all values (beginning at `idxbegin`)*/
void CValues::Read(lua_State* L, int idxbegin, int count)
{
    if (idxbegin < 0)
        idxbegin = lua_gettop(L) - idxbegin + 1; /* negative to positive index */
    dassert(lua_type(L, idxbegin) != LUA_TNONE);

    if (count == -1)
        count = lua_gettop(L); /* read all */
#ifdef SHARED_TABLES
    CValue::LuaCopiedValuesMap map;
    for (int idx = 1; idx < idxbegin + count; idx++)
    {
        dassert(lua_type(L, idx) != LUA_TNONE);
        m_values.emplace_back(L, idx, &map);
    }
#else
    for (int idx = 1; idx < idxbegin + count; idx++)
    {
        dassert(lua_type(L, idx) != LUA_TNONE);
        m_values.emplace_back(L, idx);
    }
#endif
}

void CValues::WriteAsTable(lua_State* L) const
{
    lua_createtable(L, m_values.size(), 0);
    LUA_STACK_EXPECT(0);

    int i = 1;
#ifdef SHARED_TABLES
    CValue::LuaRefList refs{ L };
    for (const auto& v : m_values)
    {
        v.Write(L, refs);
        lua_rawseti(L, -2, i++);
    }
#else
    for (const auto& v : m_values)
    {
        v.Write(L);
        lua_rawseti(L, -2, i++);
    }
#endif    
}

void CValues::Write(lua_State* L) const
{
#ifdef SHARED_TABLES
    CValue::LuaRefList refs{ L };
    for (const auto& v : m_values)
        v.Write(L, refs);
#else
    for (const auto& v : m_values)
        v.Write(L);
#endif
}

/* Push methods for different types */
#ifdef MTA_CLIENT
CValue& CValues::Push(CClientEntity* value)
#else
CValue& CValues::Push(CElement* value)
#endif
{
    return Push(CValue::UserData{ value->GetID().Value() });
}

CValue& CValues::Push(CLuaTimer* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CResource* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

#ifndef MTA_CLIENT
CValue& CValues::Push(CDbJobData* value)
{
    return Push(CValue::UserData{ value->GetId() });
}

CValue& CValues::Push(CTextItem* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CTextDisplay* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CAccount* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CAccessControlListGroup* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CAccessControlList* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
}

CValue& CValues::Push(CBan* value)
{
    return Push(CValue::UserData{ value->GetScriptID() });
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
        duration_cast<microseconds>(TimePoint::clock::now() - beginTp).count()
    );
}

/* call the function on top of the stack. Caller should clean the Lua stack */
bool CValues::CallFunctionOnStack(CLuaMain* lmain, CValues* outReturnedValues) const
{
    lua_State* const L = lmain->GetVirtualMachine();
    dassert(lua_type(L, -1) == LUA_TFUNCTION);

    const auto originalTop = lua_gettop(L) - 1; /* -1 for function on stack */
    Write(L); /* push our values to L's stack */
    lmain->ResetInstructionCount();

    bool result;
    switch (lmain->PCall(L, m_values.size(), LUA_MULTRET, 0))
    {
    case LUA_ERRRUN:
    case LUA_ERRMEM:
    {
        SString strRes = ConformResourcePath(lua_tostring(L, -1));
        GetScriptDebugging()->LogPCallError(L, strRes);
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
bool CValues::Read(NetBitStreamInterface& bitStream, CValue::TableList& tables)
{
    uint32_t nargs;
    if (!bitStream.ReadCompressed(nargs))
        return false;
    for (size_t i = 0; i < nargs; i++)
    {
        if (m_values.emplace_back().Read(bitStream, tables))
            return false;
    }
    return true;
}

bool CValues::Write(NetBitStreamInterface& bitStream, size_t& nextRef) const
{
    bitStream.WriteCompressed(static_cast<uint32_t>(m_values.size()));
    for (const auto& v : m_values)
    {
        if (!v.Write(bitStream, nextRef))
            return false;
    }
    return true;
}

/* Wrap a JSON object into a unique ptr for automatic freeing */
auto WrapJSONObject(json_object* object)
{
    const auto Deleter = [](json_object* obj) { json_object_put(obj); };
    return std::unique_ptr<json_object, decltype(Deleter)>{ object, Deleter };
}

/* JSON Read/Write */
bool CValues::Read(const char* json)
{
    /* Fast JSON check: Check first non-white space character is '[' or '{' */
    for (const char* ptr = json; *ptr; ptr++)
    {
        unsigned char c = *ptr; /* must use uchar */
        if (c == '[' || c == '{')
            break;
        if (isspace(c))
            continue;
        return false;
    }

    /* unique_ptr with custom deleter, so nobody forgets freeing */

    auto jobj = WrapJSONObject(json_tokener_parse(json));
    if (!jobj)
        return false;

    switch (json_object_get_type(jobj.get())) 
    {
    case json_type_array:
    {
        CValue::TableList tables;
        size_t len = json_object_array_length(jobj.get());
        for (size_t i = 0; i < len; i++)
        {
            json_object* ov = json_object_array_get_idx(jobj.get(), i);
            if (!m_values.emplace_back().Read(ov, tables))
                return false;
        }
        return true;
    }
    case json_type_object:
    {
        if (!m_values.emplace_back().Read(jobj.get()))
            return false;
        return true;
    }
    default:
        dassert(0); /* top level can only be an array or object as per JSON specs */
        return false;
    }
}

bool CValues::Write(std::string& json, bool serialize, int flags)
{
    auto jarray = WrapJSONObject(json_object_new_array());
    if (!jarray)
        return false;

#ifdef SHARED_TABLES
    size_t nextRef = 0;
    for (const auto& value : m_values)
    {
        if (json_object* vobj = value.Write(serialize, nextRef))
            json_object_array_add(jarray.get(), vobj);
        else
        {
            dassert(0);
            break;
        }
    }
#else
    for (const auto& value : m_values)
    {
        if (json_object* vobj = value.Write(serialize))
            json_object_array_add(jarray.get(), vobj);
        else
        {
            dassert(0);
            break;
        }
    }
#endif

    size_t length;
    json.assign(json_object_to_json_string_length(jarray.get(), flags, &length), length); /* might throw */
    /* the wrapped json object is freed automatically */
    return true;
}

}

