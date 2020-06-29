/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        sdk/game/CPedIK.h
 *  PURPOSE:     Ped inverse kinematics interface
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include <windows.h>
#include <CVector.h>

class CPedIK
{
    virtual void SetFlag(DWORD flag) = 0;
    virtual void ClearFlag(DWORD flag) = 0;
    virtual bool IsFlagSet(DWORD flag) = 0;
};
