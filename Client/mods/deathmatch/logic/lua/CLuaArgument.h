/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/lua/CLuaArgument.h
 *  PURPOSE:     Lua argument class header
 *
 *****************************************************************************/

#pragma once

extern "C"
{
    #include "lua.h"
}
#include <net/bitstream.h>
#include <string>
#include "json.h"

class CClientEntity;
class CValues;

#define LUA_TTABLEREF 9
#define LUA_TSTRING_LONG 10



// Represents a Lua value
class CValue
{
    friend class CValues;
public:
    struct None {};
    using  Nil = std::nullptr_t;

    struct TableValue : std::vector<std::pair<CValue, CValue>>
    {

        using vector::vector; /* inherit vector constructors */
        friend class CValue;
    protected:
        /* we need these functions to make the code faster */
        auto GetArrKeysCount() const { return narr; } /* get # of keys in Lua array part */
        auto SetArrKeysCount(size_t n) { narr = n; }
        auto GetRecKeysCount() const { return size() - narr; } /* same but hash part */

        void SetRef(int v) { ref = v; }
        int  GetRef() const { return ref; }

    private:
        /* When writing to bitStream this is value is used to refer to the table
        /* OR it's a Lua ref(lua_getref) to a table.
        /* Only used in `Write` functions, it's value is meaningless outside of it. */
        int ref = -1;

        /* # of keys in array part (Lua) */
        uint32_t narr = 0; 
    };
    using Table = std::shared_ptr<TableValue>;

    /* Must use list, because if realloc occurs TableRef`s will be invalidated
    * TODO: Maybe use fixed size blocks?
    * Like std::array<pair<CValue, CValue>, 16> or (better) deque?
    * Problem with deque is that it uses unpredictable amounts of memory, since
    * it's block size is implementation defined.
    * Anyways, this way, for some memory, we get potentially less cache misses, and
    * we might even save some memory, depending on the memory allocators implementation. */
    /* ACHTUNG: On table reallocation all cross table references might get reallocated
     * which might lead to random crash.
     * So do not add values manually. Always pre-allocate (with `reserve`) before
     * populating the table to make sure it doesn't reallocate.
     * The reason for using a vector stored directly in m_value, instead of a
     * shared_ptr is that this way it has better performance.
     * Also, CValue's are immutable.
     * */
    //using  Table = std::vector<std::pair<CValue, CValue>>;
    using  String = std::string;
    using  Number = lua_Number;
    using  UserData = size_t; // UserData is just an int value to either CElementIDs or CIdArray
    using  Bool = bool;
    using  TableRef = std::shared_ptr<const TableValue>;

private:
    // If `Table` is the value then we're the owner.
    // If it's a reference we aren't, but the lifetime will be the same, since we're immutable
    using Value = std::variant<String, Number, Bool, UserData, TableRef, Table, Nil, None>;

public:
    // Known values when reading from bitstream
    // It is used to save memory by not writing the same
    // table twice but rather just using a index to it (in this vector)
    using TableList = std::vector<std::weak_ptr<const TableValue>>;
    using TableToRefIDMap = CFastHashMap<TableValue const*, size_t>; /* Used when writing to stream / Lua. size_t is the index in TableList  */

    struct LuaRefList
    {
        LuaRefList(lua_State* L) : m_lua(L)
        {
            m_refs.reserve(16383); // This is most of the time a temp array, so lets make sure we wont reallocate a lot
        }

        ~LuaRefList()
        {
            for (int ref : m_refs)
                lua_unref(m_lua, ref);
        }

        [[nodiscard]] int Add()
        {
            lua_pushvalue(m_lua, -1); // luaL_ref pops the value
            return m_refs.emplace_back(luaL_ref(m_lua, LUA_REGISTRYINDEX));
        }

        void Get(size_t ref)
        {
            lua_getref(m_lua, ref);
        }

        int operator[](size_t i) const { return m_refs[i]; }

        std::vector<int> m_refs;
        lua_State* m_lua;
    };

    //using RefList = std::vector<size_t>;

    // Copied values map.
    // Used when copying CValue.
    // k - the table to be copied
    // v - the copied table
    // It is used to eliminate copied of the same table (Thus saving memory)
    using CopiedValuesMap = CFastHashMap<TableValue const*, TableRef>;
    using LuaCopiedValuesMap = CFastHashMap<const void*, std::weak_ptr<const TableValue>>;

    constexpr CValue() = default;

    template<typename T>
    CValue(T&& value) : m_value(std::forward<T>(value)) {}

    template<typename... Ts>
    CValue(Ts&&... value) : m_value(std::forward<T>(value)) { Read(std::forward<Ts>(value)...); }

    void Read(lua_State* L, int idx)
    {
        LuaCopiedValuesMap tables;
        return Read(L, idx, tables);
    }
    void Write(lua_State* L) const
    {
        LuaRefList refs{ L };
        return Write(L, refs);
    }

    bool Read(NetBitStreamInterface& bitStream)
    {
        TableList tables;
        return Read(bitStream, tables);
    }
    bool Write(NetBitStreamInterface& bitStream) const
    {
        size_t nextRef = 0;
        return Write(bitStream, nextRef);
    }

    bool Read(struct json_object* jobj)
    {
        TableList tables;
        return Read(jobj, tables);
    }
    struct json_object* Write(bool serialize) const
    {
        size_t nextRef = 0;
        return Write(serialize, nextRef);
    }


    /* Doesn't compare tables at all, just basic values */
    using ComparedTablesSet = CFastHashSet<const TableValue*>;
    bool operator==(const CValue& other) const
    {
        ComparedTablesSet set;
        return EqualTo(other, &set);
    }

    char* Write(char* buffer, size_t buffsize) const;

    template<typename Visitor>
    auto VisitValue(Visitor visitor)
    {
        return std::visit(visitor, m_value);
    }

private:
    CValue(struct json_object* jobj, TableList& tables) { bool s = Read(jobj, tables); dassert(s); }

    void Read(lua_State* L, int idx, LuaCopiedValuesMap& copiedTables);
    void Write(lua_State* L, LuaRefList& tables) const;

    bool Read(NetBitStreamInterface& bitStream, TableList& tables);
    bool Write(NetBitStreamInterface& bitStream, size_t& nextRef) const;

    bool Read(struct json_object* jobj, TableList& tables);
    struct json_object* Write(bool serialize, size_t& nextRef) const;

    bool EqualTo(const CValue& other, ComparedTablesSet* compared) const;

    Value m_value;
};

// Represents multiple values. Eg.: function arguments
class CValues
{
public:
    /*  BitStream*/
    bool Read(NetBitStreamInterface& bitStream)
    {
        CValue::TableList tables;
        return Read(bitStream, tables);
    }
    bool Write(NetBitStreamInterface& bitStream) const
    {
        size_t nextRef = 0;
        return Write(bitStream, nextRef);
    }

    /* JSON */
    bool Read(const char* json);
    bool Write(std::string& json, bool serialize = false, int flags = JSON_C_TO_STRING_PLAIN);

    /* Lua */
    void Read(struct lua_State* L, int idx = 1, int count = 1);
    void ReadAll(struct lua_State* L, int idx = 1) { Read(L, idx, -1); }
    void ReadOne(struct lua_State* L, int idx = 1) { Read(L, idx, 1); }
    void WriteAsTable(lua_State* L) const;
    void Write(struct lua_State* L) const;

    /* Push methods */
#ifdef MTA_CLIENT
    CValue& Push(class CClientEntity* value);
#else
    CValue& Push(class CDbJobData* value);
    CValue& Push(class CTextItem* value);
    CValue& Push(class CTextDisplay* value);
    CValue& Push(class CAccount* value);
    CValue& Push(class CAccessControlListGroup* value);
    CValue& Push(class CAccessControlList* value);
    CValue& Push(class CBan* value);
    CValue& Push(class CElement* value);
#endif
    CValue& Push(class CLuaTimer* value);
    CValue& Push(class CResource* value);

    //CValue& Push(CValue value) /* must copy */
    //{
    //    return m_values.emplace_back(std::move(value));
    //}

    /* Handles everything else (hopefully) */
    template<typename... Ts>
    CValue& Push(Ts&&... ts) { return m_values.emplace(std::forward<Ts>(ts)...); }

    CValue& PushNil() { return Push(nullptr); } /* life quality function */

    /* Lua function calling */
    bool Call(class CLuaMain* lmain, const CLuaFunctionRef& fn, CValues* outReturnedValues = nullptr) const;
    bool CallGlobal(class CLuaMain* lmain, const char* fn, CValues* outReturnedValues = nullptr) const;
    bool CallFunctionOnStack(class CLuaMain* lmain, CValues* outReturnedValues = nullptr) const;

    /* Functions for c++ range loop */
    auto begin() { return m_values.begin(); }
    auto begin() const { return m_values.begin(); }
    auto end() { return m_values.end(); }
    auto end() const { return m_values.end(); }


private:

    static constexpr size_t LUA_REF_TABLE_INDEX = 0;

    struct json_object* ToJSONArray(bool serialize) const;

    bool Read(NetBitStreamInterface& bitStream, CValue::TableList& tables);
    bool Write(NetBitStreamInterface& bitStream, size_t& nextRef) const;

    /* Important note: If you decide to remove elements from the list
     * you might be surpised when everything all of the sudden starts crashing.
     * The reason for that is that `CValue`s might share tables between each other.
     * If you want to change this behaviour you need to remove all `TableToRefIDMap`'s
     * where values are read from Lua.
     * If you remove that, feel free to remove every Map/List passed into CValue Read/Write
     * functions, as there will be no cross references anymore, but this might cause higher
     * memory usage.
     */
    std::list<CValue> m_values;
};
