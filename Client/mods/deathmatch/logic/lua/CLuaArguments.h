/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/lua/CLuaArguments.h
 *  PURPOSE:     Lua arguments manager class header
 *
 *****************************************************************************/

#pragma once

extern "C"
{
    #include "lua.h"
}

#include <net/bitstream.h>
#include "CLuaArgument.h"
#include "json.h"
#include <vector>
#include "CLuaFunctionRef.h"

inline void LUA_CHECKSTACK(lua_State* L, int size)
{
    if (lua_getstackgap(L) < size + 5)
        lua_checkstack(L, (size + 2) * 3 + 4);
}

class CValues;
