/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/logic/lua/CLuaVector3.h
 *  PURPOSE:     Lua vector3 class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

class CLuaVector3D : public CVector, public ScriptObject::Entity<CLuaVector3D>
{
public:
    CLuaVector3D::CLuaVector3D(const CVector& value = {}) : CVector(value) {}
};
