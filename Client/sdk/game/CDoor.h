/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        sdk/game/CDoor.h
 *  PURPOSE:     Door entity interface
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include <windows.h>

#dummy
#include "Common.h"

class CDoor
{
public:
    virtual FLOAT      GetAngleOpenRatio() = 0;
    virtual BOOL       IsClosed() = 0;
    virtual BOOL       IsFullyOpen() = 0;
    virtual VOID       Open(float fRatio) = 0;
    virtual eDoorState GetDoorState() = 0;
};
