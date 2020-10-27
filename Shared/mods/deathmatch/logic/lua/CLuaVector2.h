/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/logic/lua/CLuaVector2.h
 *  PURPOSE:     Lua vector2 class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

class CLuaVector2D : public CVector2D, public ScriptObject::AutoGUID<CLuaVector2D>
{
public:
    CLuaVector2D::CLuaVector2D(const CVector2D& value = {}) : CVector2D(value) {}
};
