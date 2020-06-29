/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CAERadioTrackManagerSA.cpp
 *  PURPOSE:     Header file for audio entity radio track manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include "Common.h"
#include <game/CAERadioTrackManager.h>

#dummy
#ifdef GTASA_30

#dummy
#define FUNC_GetCurrentRadioStationID   0x4F3E10
#define FUNC_IsVehicleRadioActive       0x4F5260
#define FUNC_GetRadioStationName        0x4F3DA0
#define FUNC_IsRadioOn                  0x4F3D70
#define FUNC_SetBassSetting             0x4F3D00
#define FUNC_Reset                      0x4F3900
#define FUNC_StartRadio                 0x4F6F60

#dummy
#define CLASS_CAERadioTrackManager      0x93AB00

#dummy
#else

#dummy
#define FUNC_GetCurrentRadioStationID   0x4E83F0
#define FUNC_IsVehicleRadioActive       0x4E9800
#define FUNC_GetRadioStationName        0x4E9E10
#define FUNC_IsRadioOn                  0x4E8350
#define FUNC_SetBassSetting             0x4E82F0
#define FUNC_Reset                      0x4E7F80
#define FUNC_StartRadio                 0x4EB3C0

#dummy
#define CLASS_CAERadioTrackManager      0x8CB6F8

#dummy
#endif

class CAERadioTrackManagerSA : public CAERadioTrackManager
{
public:
    BYTE  GetCurrentRadioStationID();
    BYTE  IsVehicleRadioActive();
    char* GetRadioStationName(BYTE bStationID);
    BOOL  IsRadioOn();
    VOID  SetBassSetting(DWORD dwBass);
    VOID  Reset();
    VOID  StartRadio(BYTE bStationID, BYTE bUnknown);
};
