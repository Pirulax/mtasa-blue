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
class CLuaArguments;

#define LUA_TTABLEREF 9
#define LUA_TSTRING_LONG 10

namespace lua
{
// Represents any value in Lua
//class CValue
//{
//    // Known values when reading from bitstream
//    using KnownTablesList = std::vector<CLuaArguments*>;
//
//    // Known tables when otherwise
//    using KnownTablesMap = CFastHashMap<CLuaArguments*, CLuaArguments*>;
//
//    constexpr CValue() = default;
//    CValue(const CValue& other, KnownTablesMap* knownTables = nullptr) :
//        m_value(other.m_value)
//    {
//    }
//    CValue(CValue&& other, KnownTablesMap* knownTables = nullptr) :
//        CValue()
//    {
//        swap(*this, other);
//    }
//    CValue(NetBitStreamInterface& bitStream, KnownTablesList* knownTables = nullptr);
//    CValue(lua_State* L, int index, KnownTablesList* knownTables = nullptr);
//
//    ~CValue() = default;
//
//
//    CValue& operator=(CValue other) { CopyAssign(std::move(other)); }
//    CValue& CopyAssign(CValue other, KnownTablesMap* knownTables = nullptr)
//    {        
//        swap(*this, other);
//    }
//
//    friend void swap(CValue& a, CValue& b)
//    {
//        using std::swap; // enable ADL
//        //swawp()        
//    }
//
//    std::variant<std::string, lua_Number, bool> m_value;
//};

// Represents a one or more value
class CValue
{
    friend class CValues;
public:
    struct None {};
    using  Nil = std::nullptr_t;

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
    using  Table = std::vector<std::pair<CValue, CValue>>;
    using  String = std::string;
    using  Number = lua_Number;
    using  UserData = size_t; // UserData is just an int value to either CElementIDs or CIdArray
    using  Bool = bool;

private:
    using TableRef = Table*;
    using ConstTableRef = Table const*;
    // If `Table` is the value then we're the owner.
    // If it's a reference we aren't, but the lifetime will be the same, since we're immutable
    using Value = std::variant<String, Number, Bool, UserData, TableRef, Table, Nil, None>;

public:
    // Known values when reading from bitstream
    // It is used to save memory by not writing the same
    // table twice but rather just using a index to it (in this vector)
    using TableList = std::vector<TableRef>;
    using TableToRefIDMap = CFastHashMap<ConstTableRef, size_t>; /* Used when writing to stream / Lua. size_t is the index in TableList  */

    // Copied values map.
    // Used when copying CValue.
    // k - the table to be copied
    // v - the copied table
    // It is used to eliminate copied of the same table (Thus saving memory)
    using CopiedValuesMap = CFastHashMap<ConstTableRef, TableRef>;
    using LuaCopiedValuesMap = CFastHashMap<const void*, TableRef>;

    constexpr CValue() = default;

    template<typename T>
    CValue(T&& value) : m_value(std::forward<T>(value)) {}
    CValue(lua_State* L, int index, LuaCopiedValuesMap* copiedTables = nullptr) { Read(L, index, copiedTables); }
    CValue(NetBitStreamInterface& bitStream, TableList* tables = nullptr) { bool s = Read(bitStream, tables); dassert(s); }

    void Read(lua_State* L, int idx, LuaCopiedValuesMap* copiedTables = nullptr);
    void Write(lua_State* L, TableToRefIDMap* tables = nullptr) const;
    void Write(lua_State* L)
    {
        TableToRefIDMap map;
        /* Free all lua_ref's */
        for (const auto& [k, v] : map)
            luaL_unref(L, LUA_REGISTRYINDEX, v);
        return Write(L, &map);
    }

    bool Read(NetBitStreamInterface& bitStream, TableList* tables = nullptr);
    bool Write(NetBitStreamInterface& bitStream, TableToRefIDMap* tables = nullptr) const;

    bool Read(struct json_object* jobj, TableList& tables);
    bool Read(struct json_object* jobj)
    {
        TableList tables;
        return Read(jobj, tables);
    }
    struct json_object* Write(bool serialize, TableToRefIDMap& tableToRef) const;
    struct json_object* Write(bool serialize) const
    {
        TableToRefIDMap map;
        return Write(serialize, map);
    }


    /* Doesn't compare tables at all, just basic values */
    using ComparedTablesSet = CFastHashSet<ConstTableRef>;
    bool operator==(const CValue& other) const
    {
        ComparedTablesSet set;
        return EqualTo(other, &set);
    }

    char* Write(char* buffer, size_t buffsize) const;

private:
    CValue(struct json_object* jobj, TableList& tables) { bool s = Read(jobj, tables); dassert(s); }


    bool EqualTo(const CValue& other, ComparedTablesSet* compared) const;

    Value m_value;
};

class CValues
{
public:
    /* Lua */
    void ReadOne(struct lua_State* L, int idx) { m_values.emplace_back(L, idx); }
    void ReadAll(struct lua_State* L, int idx = 1);


    /*  BitStream*/
    bool Read(NetBitStreamInterface& bitStream)
    {
        CValue::TableList tables;
        return Read(bitStream, tables);
    }
    bool Write(NetBitStreamInterface& bitStream) const
    {
        CValue::TableToRefIDMap map;
        return Write(bitStream, map);
    }

    /* JSON */
    bool Read(const char* json);
    bool Write(std::string& json, bool serialize = false, int flags = JSON_C_TO_STRING_PLAIN);

    void Push(struct lua_State* L);

    /* Functions for c++ range loop */
    auto begin() { return m_values.begin(); }
    auto begin() const { return m_values.begin(); }
    auto end() { return m_values.end(); }
    auto end() const { return m_values.end(); }

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
    template<typename T>
    CValue& Push(T&& value)
    {
        return m_values.emplace(std::forward<T>(value));
    }

private:
    struct json_object* ToJSONArray(bool serialize) const;

    bool Read(NetBitStreamInterface& bitStream, CValue::TableList& tables);
    bool Write(NetBitStreamInterface& bitStream, CValue::TableToRefIDMap& tables) const;

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

};

class CLuaArgument
{
public:
    CLuaArgument();
    CLuaArgument(const CLuaArgument& Argument, CFastHashMap<CLuaArguments*, CLuaArguments*>* pKnownTables = NULL);
    CLuaArgument(NetBitStreamInterface& bitStream, std::vector<CLuaArguments*>* pKnownTables = NULL);
    CLuaArgument(lua_State* luaVM, int iArgument, CFastHashMap<const void*, CLuaArguments*>* pKnownTables = NULL);
    ~CLuaArgument();

    const CLuaArgument& operator=(const CLuaArgument& Argument);
    bool                operator==(const CLuaArgument& Argument);
    bool                operator!=(const CLuaArgument& Argument);

    void Read(lua_State* luaVM, int iArgument, CFastHashMap<const void*, CLuaArguments*>* pKnownTables = NULL);
    void ReadBool(bool bBool);
    void ReadNumber(double dNumber);
    void ReadString(const std::string& strString);
    void ReadElement(CClientEntity* pElement);
    void ReadScriptID(uint uiScriptID);
    void ReadElementID(ElementID ID);
    void ReadTable(class CLuaArguments* table);

    void Push(lua_State* luaVM, CFastHashMap<CLuaArguments*, int>* pKnownTables = NULL) const;

    int GetType() const { return m_iType; };
    int GetIndex() const { return m_iIndex; };

    bool           GetBoolean() const { return m_bBoolean; };
    lua_Number     GetNumber() const { return m_Number; };
    const SString& GetString() { return m_strString; };
    void*          GetUserData() const { return m_pUserData; };
    CClientEntity* GetElement() const;

    bool         ReadFromBitStream(NetBitStreamInterface& bitStream, std::vector<CLuaArguments*>* pKnownTables = NULL);
    bool         WriteToBitStream(NetBitStreamInterface& bitStream, CFastHashMap<CLuaArguments*, unsigned long>* pKnownTables = NULL) const;
    json_object* WriteToJSONObject(bool bSerialize = false, CFastHashMap<CLuaArguments*, unsigned long>* pKnownTables = NULL);
    bool         ReadFromJSONObject(json_object* object, std::vector<CLuaArguments*>* pKnownTables = NULL);
    char*        WriteToString(char* szBuffer, int length);

private:
    void LogUnableToPacketize(const char* szMessage) const;

    int            m_iType;
    int            m_iIndex;
    bool           m_bBoolean;
    lua_Number     m_Number;
    SString        m_strString;
    void*          m_pUserData;
    CLuaArguments* m_pTableData;
    bool           m_bWeakTableRef;

#ifdef MTA_DEBUG
    std::string m_strFilename;
    int         m_iLine;
#endif

    void CopyRecursive(const CLuaArgument& Argument, CFastHashMap<CLuaArguments*, CLuaArguments*>* pKnownTables = NULL);
    bool CompareRecursive(const CLuaArgument& Argument, std::set<CLuaArguments*>* pKnownTables = NULL);
    void DeleteTableData();
};
