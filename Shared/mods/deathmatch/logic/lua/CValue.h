#pragma once

extern "C"
{
    #include "lua.h"
}
#include "RefList.h"
#include <net/bitstream.h>
#include <string>
#include <variant>
#include <memory>
#include <utility>
#include "json.h"
#include <SharedUtil.FastHashMap.h>

struct lua_State;
struct json_object;

#ifdef MTA_CLIENT
class CClientEntity;
#else
class CElement;
#endif
class CLuaArguments;

#define LUA_TTABLEREF 9
#define LUA_TSTRING_LONG 10

namespace lua
{
/* * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class representing a Lua value.                 *
 * Once constructed their value can not be changed *
\* * * * * * * * * * * * * * * * * * * * * * * * * */
class CValue
{
    friend class CValues;
public:
    struct UnableToPacketizeError : public std::runtime_error
    {
        using runtime_error::runtime_error;
        UnableToPacketizeError() :
            runtime_error("Unable to packetize")
        {
        }

        ~UnableToPacketizeError() {}
    };
public:
    using None = std::monostate;
    using Nil = std::nullptr_t;
    using String = std::string;
    using Number = lua_Number;
    using UserData = size_t; // UserData is just an index to either CElementIDs or CIdArray
    using Bool = bool;

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
     * TODO: This isn't correct anymore, as I switched to using shared_ptr   *
     * Table's value is immuatble once it is set (in CValue constructor).    *
     * This is because table elements might refer to each other (TableRef),  *
     * on insertion the vector might reallocate, which would invalidate      *
     * all xrefs.                                                            *
     * Only function not belonging to `std::vector` can be used since        *
     * they don't cause reallocating, nor invalidate any xrefs.              *
    \* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    struct Table : std::vector<std::pair<CValue, CValue>>
    {

        using vector::vector; /* inherit vector constructors */
        friend class CValue;
    protected:
        // TODO - Find a good way to get exact size of array and hash part without modifying Lua
        //auto GetArrKeysCount() const { return narr; } /* get # of keys in Lua array part */
        //auto SetArrKeysCount(size_t n) { narr = n; }
        //auto GetRecKeysCount() const { return size() - narr; } /* same but hash part */

        void SetRef(int v) { ref = v; }
        int  GetRef() const { return ref; }

    private:
        /* When writing to bitStream this is value is used to refer to the table
        /* OR it's a Lua ref(lua_getref) to a table.
        /* Only used in `Write` functions, it's value is meaningless outside of it. */
        int ref = -1;

        /* # of keys in array part (Lua) */
        //uint32_t narr = 0; 
    };

private:
    friend Table; // Let this class use our hidden ctors

    using OwnedTable = std::shared_ptr<Table>; 
    using TableRef = std::shared_ptr<const Table>;
    using WeakTableRef = TableRef::weak_type;
    static_assert(!std::is_same_v<OwnedTable, TableRef>); // ...otherwise code won't work as expected.

    // None has to be the first value, to make the variant have None value by default
    using Value = std::variant<None, String, Number, Bool, UserData, TableRef, OwnedTable, Nil>;
public:
    // Used for bitstream read/write.
    // When a table is read for the first time it's assigned a reference.
    // The reference is an index into this vector.
    // The value of the refernece is the vector's size when the refrence is created.
    // This saves memory if a table is refernced multiple times.
    using ReferencedTables = std::vector<TableRef>;

    // Copied values map.
    // Used when copying CValue.
    // k - the table to be copied (
    // v - the copied table
    // It is used to eliminate copied of the same table (Thus saving memory)
    // Also, this way original references remain valid (Since table's aren't copied in Lua)
    using CopiedValuesMap = CFastHashMap<TableRef::element_type*, TableRef>;

    // Same as above, but used when reading values from Lua.
    // k - pointer to table obtained with lua_topointer
    // v - copied table
    using LuaCopiedValuesMap = CFastHashMap<const void*, TableRef>;


    CValue() = default;

    explicit CValue(UserData value) : m_value(value) {} // Should only be used in CValues. Has to be declared public as STL containers can't use it otherwise
    explicit CValue(std::string_view view) : m_value(String{ view }) {}
    explicit CValue(String value) : m_value(std::move(value)) {}
    explicit CValue(Number value) : m_value(value) {}
    explicit CValue(bool value) : m_value(value) {}
    explicit CValue(Nil) : m_value(Nil{}) {}

    // Sadly these wrappers are inevitable...
    CValue(const CValue& other)
    {
        CopiedValuesMap map;
        CValue value{ other, map };
        swap(*this, value);
    }
    CValue(CValue&& other) noexcept :
        CValue()
    {
        swap(*this, other);
    }

    CValue(NetBitStreamInterface& bitStream) :
        CValue()
    {
        ReferencedTables tbls;
        CValue value(bitStream, tbls);
        swap(*this, value);
    }
    CValue(lua_State* L, int idx) :
        CValue()
    {
        LuaCopiedValuesMap map;
        CValue value(L, idx, map);
        swap(*this, value);
    }
    CValue(json_object* jobj) :
        CValue()
    {
        ReferencedTables map;
        CValue value(jobj, map);
        swap(*this, value);
    }

    void Write(lua_State* L) const;

    void Write(NetBitStreamInterface& bitStream) const
    {
        size_t nextRef = 0;
        return Write(bitStream, nextRef);
    }

    /* used by CValues only */
    json_object* Write(bool serialize) const
    {
        size_t nextRef = 0;
        return Write(serialize, nextRef);
    }


    /* Doesn't compare tables */
    bool FlatCompare(const CValue& other) const;

    /* Getters */

    // Wrapper around std::get<T>(variant)
    template<typename T>
    const auto& Get() const
    {
        if constexpr (std::is_same_v<T, Table>)
        {
            if (std::holds_alternative<TableRef>(m_value))
                return *std::get<TableRef>(m_value);
            else
                return *std::get<OwnedTable>(m_value);
        }
        else
            return std::get<T>(m_value);
    }

    template<typename T>
    bool Holds() const
    {
        if constexpr (std::is_same_v<T, Table>)
            return std::holds_alternative<TableRef>(m_value) || std::holds_alternative<OwnedTable>(m_value);
        else
            return std::holds_alternative<T>(m_value);
    }

    template<typename Visitor>
    auto Visit(Visitor visitor) const
    {
        return std::visit(visitor, [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, OwnedTable> || std::is_same_v<T, TableRef>)
                return visitor(const_cast<const Table&>(*value));
            else
                return visitor(value);
        });
    }

    friend void swap(CValue& lhs, CValue& rhs)
    {
        if (&lhs == &rhs)
            return;

        using std::swap;
        swap(lhs.m_value, rhs.m_value);
    }
private:
    CValue(const CValue& other, CopiedValuesMap& map) {} // TODO

    CValue(lua_State* L, int idx, LuaCopiedValuesMap& copiedTables);
    void Write(lua_State* L, RefList& tables) const;

    CValue(NetBitStreamInterface& bitStream, ReferencedTables& tables);
    void Write(NetBitStreamInterface& bitStream, size_t& nextRef) const;

    CValue(json_object* jobj, ReferencedTables& tables);
    json_object* Write(bool serialize, size_t& nextRef) const;

    /* Internal usage only */
    void ToJSONKey(char* buffer, size_t buffsize) const;

    Value m_value = Nil{};
};
};
