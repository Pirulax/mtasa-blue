/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/deathmatch/logic/luadefs/CLuaFileDefs.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once
#include "luadefs/CLuaDefs.h"

class CLuaFileDefs : public CLuaDefs
{
public:
    static void LoadFunctions();
    static void AddClass(lua_State* luaVM);

private:
    LUA_DECLARE(File);
    LUA_DECLARE(fileOpen);
    LUA_DECLARE(fileCreate);
    LUA_DECLARE(fileExists);
    LUA_DECLARE(fileCopy);
    LUA_DECLARE(fileRename);
    LUA_DECLARE(fileDelete);

    LUA_DECLARE(fileClose);
    LUA_DECLARE(fileFlush);
    static std::string fileRead(lua_State* L, CScriptFile* pFile, size_t count);
    LUA_DECLARE(fileWrite);

    LUA_DECLARE(fileGetPos);
    LUA_DECLARE(fileGetSize);
    LUA_DECLARE(fileGetPath);
    LUA_DECLARE(fileIsEOF);

    LUA_DECLARE(fileSetPos);

    LUA_DECLARE(fileCloseGC);
};
