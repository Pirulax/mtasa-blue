/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/logic/lua/CLuaMatrix.h
 *  PURPOSE:     Lua matrix class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

class CLuaMatrix : public CMatrix, public ScriptObject::Entity<CLuaMatrix>
{
public:
    CLuaMatrix::CLuaMatrix(const CMatrix& matrix = {}) : CMatrix(matrix) {}
};
