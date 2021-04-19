#include <StdInc.h>
#include "CLuaUnitTestDefs.h"
#include <lua/LuaBasic.h>

#include <lua/CValues.h>
#include <lua/CLuaStackChecker.h>
#include <net/bitstream.h>

#define MTA_UNITTEST
#ifdef MTA_UNITTEST
using namespace lua;

// CLuaArgs <=> CValues test helpers

// Lua stack -> CValues -> Lua stack
int LuaValuesReadWrite(lua_State* L)
{
    CValues read(L, 1, -1);
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
    CBitStream bitStream;
    CValues(L, 1, -1).Write(*bitStream.pBitStream); // Read from stack, write to stream

    CValues read(*bitStream.pBitStream);
    read.Write(L); // Read from stream, write to stack

    return static_cast<int>(read.Count());
}

// Lua stack -> CLuaArgs -> BitStream -> CLuaArgs -> Lua stack
int LuaArgsReadWriteBitStream(lua_State* L)
{
    CBitStream bitStream;

    // Write to BS with LuaArgs
    CLuaArguments readStack;
    readStack.ReadArguments(L, 1);
    readStack.WriteToBitStream(*bitStream.pBitStream);

    // Read from BS with LuaArgs
    CLuaArguments args;
    args.ReadFromBitStream(*bitStream.pBitStream);
    args.PushArguments(L);

    return static_cast<int>(args.Count());
}

// Lua stack -> CLuaArgs -> BitStream -> CValues -> Lua Stack
int LuaArgsReadValuesWriteBitStream(lua_State* L)
{
    CBitStream bitStream;

    // Write to BS with LuaArgs
    CLuaArguments args;
    args.ReadArguments(L, 1);
    args.WriteToBitStream(*bitStream.pBitStream);

    // Read from BS with CValues
    CValues read(*bitStream.pBitStream);
    read.Write(L);
    return static_cast<int>(read.Count());
}

// Lua stack -> CValues -> BitStream -> CLuaArgs -> Lua Stack
int LuaValuesReadArgsWriteBitStream(lua_State* L)
{
    CBitStream bitStream;

    // Write to BS with CValues
    CValues(L, 1, -1).Write(*bitStream.pBitStream);

    // Read from BS with LuaArgs
    CLuaArguments args;
    args.ReadFromBitStream(*bitStream.pBitStream);
    args.PushArguments(L);

    return static_cast<int>(args.Count());
}


// Lua stack -> CValues -> JSON string
int LuaValuesToJSON(lua_State* L)
{
    std::string json;
    CValues(L, 1, -1).Write(json);
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
#ifdef MTA_CLIENT
        g_pClientGame->GetScriptDebugging()->LogError(L, "%s", e.what());
#else
        g_pGame->GetScriptDebugging()->LogError(L, "%s", e.what());
#endif
        return 0;
    }
}
void CLuaUnitTestDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"LuaValuesReadWrite", LuaWrappedTryCatch<LuaValuesReadWrite>},
        {"LuaArgsReadWrite", LuaWrappedTryCatch<LuaArgsReadWrite>},

        {"LuaValuesReadWriteBitStream", LuaWrappedTryCatch<LuaValuesReadWriteBitStream>},
        {"LuaArgsReadWriteBitStream", LuaWrappedTryCatch<LuaArgsReadWriteBitStream>},
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
#else
void CLuaUnitTestDefs::LoadFunctions()
{
}
#endif
