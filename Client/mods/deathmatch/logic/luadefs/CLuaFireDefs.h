/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/mods/deathmatch/logic/luadefs/CLuaFireDefs.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/
#dummy
#pragma once

#dummy
#include "CLuaDefs.h"

class CLuaFireDefs : public CLuaDefs
{
public:
    static void LoadFunctions();

    LUA_DECLARE(CreateFire);
    LUA_DECLARE(ExtinguishFire);
};
