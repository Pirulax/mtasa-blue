#include <StdInc.h>
#include "CLuaUnitTestDefs.h"
#include <lua/LuaBasic.h>

#include <lua/CValues.h>
#include <lua/CLuaStackChecker.h>

//// Values to test with should be on the stack
//// This function does some checks, then
//// pushes the data back thru CValues
//int TestLuaArgs(lua_State* L)
//{
//    lua::CValues values(L, -1, -1); // Read all data from the stack
//
//    
//    {
//
//    }
//}
//
//void DoLuaArgsInitialTests(lua_State* L)
//{
//    lua_settop(L, 0); // Clear stack
//
//    LUA_STACK_EXPECT(0);
//
//    using namespace lua;
//    {
//        // Test basic value types
//        const auto PushReadCheck = [L](auto valueToRead) {
//            LUA_STACK_EXPECT(0);
//
//            using T = decltype(valueToRead);
//
//            Push(L, std::move(valueToRead));
//            CValue read(L, -1);
//            lua_pop(L, 1);
//            assert(read.Get<T>() == valueToRead);
//        };
//
//        PushReadCheck(true);
//        PushReadCheck(false);
//
//        PushReadCheck(CValue::String{ "String" });
//        PushReadCheck(CValue::String{});
//
//        PushReadCheck(CValue::Number{121234.4123231});
//    }
//
//    {
//        LUA_STACK_EXPECT(0);
//
//        // Test multiple values
//        Push(L, 1.2112);
//        Push(L, true);
//        Push(L, std::string_view{ "Test" }); // Incorrectly resolves to Push<bool>
//        Push(L, std::vector<CValue::Number>{}); // Table
//        Push(L, CVector{ 1.f, 2.f, 3.f }); // Userdata
//
//        CValues values(L, 1, -1); // Read all values on tack
//        assert(values.Count() == lua_gettop(L));
//
//        // Iter on value, see if everything is as expected
//        // We only want to check if the order and types are correct
//        int i = 1;
//        for (const CValue& iterValue : values)
//        {
//            CValue expected(L, i);
//            expected.Visit([&](const auto& value) {
//                using T = std::decay_t<decltype(value)>;
//                assert(iterValue.Holds<T>());
//            });
//            i++;
//        }
//
//        lua_settop(L, 0); // Clear stack
//    }
//
//    {
//        LUA_STACK_EXPECT(0);
//
//        // Test userdata
//        Push(L, CVector(1.f, 2.f, 3.f));
//        CValue read(L, -1);
//        CValue::UserData idx = read.Get<CValue::UserData>(); // Userdatas just store an index to CIdArray / CElementIDs
//        assert(idx == reinterpret_cast<size_t>(lua_touserdata(L, -1)));
//        lua_pop(L, 1);
//    }
//
//    {
//        LUA_STACK_EXPECT(0);
//
//        // Table
//        std::vector<CValue::Number> values{ 1.1, 1.2, 1000000.0 };
//        Push(L, values); // Table on stack
//
//        const CValue::Table& table = CValue(L, -1).Get<CValue::Table>();
//        assert(table.size() == values.size());
//        assert(std::equal(values.begin(), values.end(), table.begin(),
//            [](CValue::Number number, const CValue& value) {
//                return number == value.Get<CValue::Number>();
//            }
//        ));
//    }
//
//    {
//        const auto WriteReadBitStream = [L](auto... values) {
//            (Push(L, values), ...);
//
//            auto& bitStream = *g_pNet->AllocateNetBitStream();
//
//            CValues readFromStack(L, 1);
//            readFromStack.Write(bitStream);
//            CValues readFromBS(bitStream);
//            assert(readFromStack.FlatCompare(readFromBS)); // DOESN'T TEST TABLES!!
//
//            g_pNet->DeallocateNetBitStream(&bitStream);
//            lua_settop(L, 0);
//        };
//
//        WriteReadBitStream(1.2121);
//        WriteReadBitStream(true);
//        WriteReadBitStream(std::string_view{ "Yo papa" });
//
//        // Multiple numbers
//        WriteReadBitStream(1.2121, 1.0, 2.2, 3.3, 4.4);
//
//        // True/false
//        WriteReadBitStream(true, false);
//
//        // Multiple strings
//        WriteReadBitStream(
//            std::string_view{ "Yo papa" },
//            std::string_view{ "Ayyo papa" },
//            std::string_view{ "Calm down man" }
//        );
//
//
//        WriteReadBitStream(
//            std::vector<std::vector<CValue::Number>>{
//                { 1.1, 2.2, 3.3 },
//                { 4.4, 5.5, 6.6, 7.7, 8.8 },
//                { 9.9, 10.10, 11.11, 12.12 }
//        }
//        );
//
//
//        WriteReadBitStream(
//            1.2121,
//            true,
//            std::string_view{ "Test" },
//            std::vector<CValue::Number>{}
//        );
//
//
//        {
//            LUA_STACK_EXPECT(0);
//
//            Push(L, 1.2112);
//            Push(L, true);
//            Push(L, std::string_view{ "Test" }); // Incorrectly resolves to Push<bool>
//            Push(L, std::vector<CValue::Number>{}); // Table
//            // Push(L, CVector{ 1.f, 2.f, 3.f }); // Userdata - Only elements
//
//
//            auto& bitStream = *g_pNet->AllocateNetBitStream();
//            CValues readFromStack(L, 1);
//            readFromStack.Write(bitStream);
//            CValues readFromBS(bitStream);
//            assert(readFromStack.FlatCompare(readFromBS));
//            g_pNet->DeallocateNetBitStream(&bitStream);
//
//
//        }
//    }
//}

using namespace lua;

// Lua stack -> CValues -> Lua stack
int LuaValuesReadWrite(lua_State* L)
{
    CValues read(L, 1);
    read.Write(L);
    return static_cast<int>(read.Count());
}

// Lua stack -> CArgs -> Lua stack
int LuaArgsReadWrite(lua_State* L)
{
    CLuaArguments args;
    args.ReadArguments(L, 1);
    args.PushArguments(L);
    return args.Count();
}


// Lua stack -> CValues -> BitStream -> CValues -> Lua stack
int LuaValuesReadWriteBitStream(lua_State* L)
{
    auto& bitStream = *g_pNet->AllocateNetBitStream();

    CValues(L, 1).Write(bitStream); // Read from stack, write to stream

    CValues read(bitStream);
    read.Write(L); // Read from stream, write to stack

    g_pNet->DeallocateNetBitStream(&bitStream);
    return static_cast<int>(read.Count());
}

// Lua stack -> CLuaArgs -> BitStream -> CValues -> Lua Stack
int LuaArgsReadValuesWriteBitStream(lua_State* L)
{
    auto& bitStream = *g_pNet->AllocateNetBitStream();

    // Write to BS with LuaArgs
    CLuaArguments args;
    args.ReadArguments(L, 1);
    args.WriteToBitStream(bitStream);

    // Read from BS with CValues
    CValues read(bitStream);
    read.Write(L);

    g_pNet->DeallocateNetBitStream(&bitStream);
    return static_cast<int>(read.Count());
}

// Lua stack -> CValues -> BitStream -> CLuaArgs -> Lua Stack
int LuaValuesReadArgsWriteBitStream(lua_State* L)
{
    auto& bitStream = *g_pNet->AllocateNetBitStream();

    // Write to BS with CValues
    CValues(L, 1).Write(bitStream);

    // Read from BS with LuaArgs
    CLuaArguments args;
    args.PushArguments(bitStream);

    g_pNet->DeallocateNetBitStream(&bitStream);
    return static_cast<int>(args.Count());
}


// Lua stack -> CValues -> JSON string
int LuaValuesToJSON(lua_State* L)
{
    std::string json;
    CValues(L, 1).Write(json);
    Push(L, json);
    return 1;
}

// JSON string -> CValues -> Lua stack
int LuaValuesFromJSON(lua_State* L)
{
    const char* json = lua_tostring(L, -1);
    CValues read(json);
    read.Write(L);
    return static_cast<int>(read.Count());
}

// Lua stack -> CLuaArgs -> JSON string
int LuaArgsToJSON(lua_State* L)
{
    CLuaArguments args;
    args.ReadArguments(L, 1);
    std::string json;
    args.WriteToJSONString(json);
    Push(L, json);
    return 1;
}

// JSON string -> CLuaArgs
int LuaArgsFromJSON(lua_State* L)
{
    CLuaArguments args;
    const char* json = lua_tostring(L, -1);
    args.ReadFromJSONString(json);
    args.PushArguments(L);
    return args.Count();
}


// CValues functions throw
template<auto Fn>
int LuaWrappedTryCatch(lua_State* L)
{
    try
    {
        return Fn(L);
    }
    catch (const std::exception& e)
    {
        g_pClientGame->GetScriptDebugging()->LogError(L, "%s", e.what());
        return 0;
    }
}
void CLuaUnitTestDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"LuaValuesReadWrite", LuaWrappedTryCatch<LuaValuesReadWrite>},
        {"LuaArgsReadWrite", LuaWrappedTryCatch<LuaArgsReadWrite>},

        {"LuaValuesReadWriteBitStream", LuaWrappedTryCatch<LuaValuesReadWriteBitStream>},
        {"LuaArgsReadValuesWriteBitStream", LuaWrappedTryCatch<LuaArgsReadValuesWriteBitStream>},
        {"LuaValuesReadArgsWriteBitStream", LuaWrappedTryCatch<LuaValuesReadArgsWriteBitStream>},

        {"LuaValuesToJSON", LuaWrappedTryCatch<LuaValuesToJSON>},
        {"LuaValuesFromJSON", LuaWrappedTryCatch<LuaValuesFromJSON>},
        {"LuaArgsToJSON", LuaWrappedTryCatch<LuaArgsToJSON>},
        {"LuaArgsFromJSON", LuaWrappedTryCatch<LuaArgsFromJSON>},
    };

    // Add functions
    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}
