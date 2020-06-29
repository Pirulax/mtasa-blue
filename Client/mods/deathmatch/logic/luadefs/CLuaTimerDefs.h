/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/luadefs/CLuaTimerDefs.cpp
 *  PURPOSE:     Lua definitions class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once
#include "CLuaDefs.h"

class CLuaTimerDefs : public CLuaDefs
{
public:
    static void LoadFunctions();
    static void AddClass(lua_State* luaVM);

    LUA_DECLARE(SetTimer);
    LUA_DECLARE(KillTimer);
    LUA_DECLARE(ResetTimer);
    LUA_DECLARE(GetTimers);
    LUA_DECLARE(IsTimer);
    LUA_DECLARE(GetTimerDetails);
};
