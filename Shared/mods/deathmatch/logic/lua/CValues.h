#pragma once

#include "CValue.h"
// Represents multiple values. Eg.: function arguments

class CLuaFunctionRef;
namespace lua
{
class CValues
{
public:
    template<typename... Args>
    CValues(Args&&... args)
    {
        Read(std::forward<Args>(args)...);
    }

    /* Bit Stream */
    void Read(NetBitStreamInterface& bitStream);
    void Write(NetBitStreamInterface& bitStream) const;

    /* JSON */
    void Read(const char* json);
    void Write(std::string& json, bool serialize = false, int flags = JSON_C_TO_STRING_PLAIN);

    /* Lua */
    void Read(struct lua_State* L, int idxbegin = 1, int count = 1);
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
    CValue& Push(Ts&&... ts) { return m_values.emplace_back(std::forward<Ts>(ts)...); }

    /* Lua function calling */
    bool Call(class CLuaMain* lmain, const CLuaFunctionRef& fn, CValues* outReturnedValues = nullptr) const;
    bool CallGlobal(class CLuaMain* lmain, const char* fn, CValues* outReturnedValues = nullptr) const;
    bool CallFunctionOnStack(class CLuaMain* lmain, CValues* outReturnedValues = nullptr) const;

    /* Functions for c++ range loop */
    auto begin() { return m_values.begin(); }
    auto begin() const { return m_values.begin(); }
    auto end() { return m_values.end(); }
    auto end() const { return m_values.end(); }

    size_t Count() const { return m_values.size(); }
    const CValue& Front() const { return m_values.front(); } // Values can't be modified once read
    const CValue& Back() const { return m_values.back(); }
    const CValue& operator[](size_t i) const { return *std::next(begin(), i); } // Possibly inefficient

    bool FlatCompare(const CValues& other)
    {
        if (other.Count() != Count())
            return false;
        return std::equal(other.begin(), other.end(), m_values.begin(), [](const CValue& a, const CValue& b) {
            return a.FlatCompare(b);
        });
    }
private:
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
