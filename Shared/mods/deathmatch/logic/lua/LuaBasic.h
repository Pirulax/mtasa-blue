/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/
#pragma once
#include <optional>
#include <variant>

/*
    Basic Lua operations:
        int Push(L, T value)
        T PopPrimitive(L, std::size_t stackIndex)
*/

class CVector2D;
class CVector;
class CVector4D;

namespace lua
{
    // Implementation details, not to be used from outside `namespace lua`
    namespace detail
    {
        // Pack n values from the top of the stack into an array
        inline int PackValuesIntoArray(lua_State* L, int n)
        {
            const int t = lua_gettop(L) - n + 1;
            lua_createtable(L, n, 0);   // Push array on top
            lua_insert(L, t);           // Insert table below the values (because Lua pops from the top)

            // Pack values into array
            // This loop is reversed, because the top of the 
            // stack needs to be the last value in the array
            for (int i = n; i >= 1; i--)
                lua_rawseti(L, t, i);

            return 1;
        }

        // Ensures T will be a single value on the stack
        template<typename T>
        inline int PushAsSingleValue(lua_State* L, const T& v)
        {
            if (Push(L, v) > 1)
                PackValuesIntoArray(L, n);
            return 1;
        }
    }

    // PopTrival should read a simple value of type T from the stack without extra type checks
    // If whatever is at that point in the stack is not convertible to T, the behavior is undefined
    template <typename T>
    inline T PopPrimitive(lua_State* L, std::size_t& index);


    // Push should push a value of type T to the Lua Stack
    // The return value must be the net amount of items pushed to the stack, which should
    // be 1 for most types (e.g. Push<int>) but may be any number for special cases
    // like tuples, in order to allow returning multiple values from a function
    inline int Push(lua_State* L, int value)
    {
        lua_pushnumber(L, value);
        return 1;
    }
    inline int Push(lua_State* L, unsigned int value)
    {
        lua_pushnumber(L, value);
        return 1;
    }
    inline int Push(lua_State* L, float value)
    {
        lua_pushnumber(L, value);
        return 1;
    }
    inline int Push(lua_State* L, double value)
    {
        lua_pushnumber(L, value);
        return 1;
    }

    inline int Push(lua_State* L, bool value)
    {
        lua_pushboolean(L, value);
        return 1;
    }

    inline int Push(lua_State* L, nullptr_t)
    {
        lua_pushnil(L);
        return 1;
    }

    inline int Push(lua_State* L, const std::string& value)
    {
        lua_pushlstring(L, value.data(), value.length());
        return 1;
    }

    inline int Push(lua_State* L, const CLuaArgument& arg)
    {
        if (arg.GetType() == LUA_TNONE)
            return 0;

        arg.Push(L);
        return 1;
    }

    inline int Push(lua_State* L, const CLuaArguments& args)
    {
        args.PushAsTable(L);
        return 1;
    }

    inline int Push(lua_State* L, const CVector2D& value)
    {
        lua_pushvector(L, value);
        return 1;
    }

    inline int Push(lua_State* L, const CVector& value)
    {
        lua_pushvector(L, value);
        return 1;
    }

    inline int Push(lua_State* L, const CVector4D& value)
    {
        lua_pushvector(L, value);
        return 1;
    }

    inline int Push(lua_State* L, const CMatrix& value)
    {
        lua_pushmatrix(L, value);
        return 1;
    }

    // Overload for pointers to classes. We boldly assume that these are script entities
    template <typename T>
    typename std::enable_if_t<(std::is_pointer_v<T>&& std::is_class_v<std::remove_pointer_t<T>>), int> Push(lua_State* L, const T&& val)
    {
        lua_pushelement(L, val);
        return 1;
    }

    template <typename T>
    int Push(lua_State* L, const std::optional<T>& val)
    {
        if (val.has_value())
            return Push(L, val.value());
        else
            return Push(L, nullptr);
    }

    template <typename... Ts>
    int Push(lua_State* L, const std::variant<Ts...>& val)
    {
        return std::visit([L](const auto& value) { return Push(L, value); }, val);
    }

    template <typename T>
    int Push(lua_State* L, const std::vector<T>& val)
    {
        lua_createtable(L, val.size(), 0);

        int i = 1;
        for (const T& v : val)
        {
            detail::PushAsSingleValue(L, v);
            lua_rawseti(L, -2, i++);
        }

        // Only the table remains on the stack
        return 1;
    }

    template <typename T, size_t N>
    int Push(lua_State* L, const std::array<T, N>& val)
    {
        lua_createtable(L, N, 0);

        int i = 1;
        for (const T& v : val)
        {
            detail::PushAsSingleValue(L, v);
            lua_rawseti(L, -2, i++);
        }

        return 1;
    }

    template <typename K, typename V>
    int Push(lua_State* L, const std::unordered_map<K, V>& val)
    {
        lua_createtable(L, 0, val.size());

        for (const auto& [k, v] : val)
        {
            detail::PushAsSingleValue(L, k); // Use this if for any reason someone uses tuple as the key
            detail::PushAsSingleValue(L, v);
            lua_rawset(L, -3);
        }

        // Only the table remains on the stack
        return 1;
    }

    // Tuples can be used to return multiple results or
    // if used as an array value, its values will be packed into an array
    // (Note: The packing of values into an array is done in `PushAsSingleValue`)
    template<typename... Ts>
    int Push(lua_State* L, const std::tuple<Ts...>& tuple)
    {
        // Call Push on each element of the tuple
        // Returns how many values in total have been pushed onto the stack
        return std::apply([L](const auto&... value) { return (Push(L, value) + ...); }, tuple);
    }

    // Overload for enum types only
    template <typename T>
    typename std::enable_if_t<std::is_enum_v<T>, int> Push(lua_State* L, const T&& val)
    {
        return Push(L, EnumToString(val));
    }
}
