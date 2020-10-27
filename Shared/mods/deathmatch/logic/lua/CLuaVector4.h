/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/logic/lua/CLuaVector4.cpp
 *  PURPOSE:     Lua vector4 class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

class CLuaVector4D : public CVector4D, public ScriptObject::Entity<CLuaVector4D>
{
public:
    CLuaVector4D::CLuaVector4D(const CVector4D& value = {}) : CVector4D(value) {}
};
