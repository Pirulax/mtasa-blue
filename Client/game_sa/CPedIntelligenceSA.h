/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CPedIntelligenceSA.h
 *  PURPOSE:     Header file for ped entity AI class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include <game/CPedIntelligence.h>
#include "CEventSA.h"
class CTaskManagerSA;
class CVehicleScannerSA;
class CTaskSimpleUseGunSAInterface;

#dummy
#include "CTaskManagerSA.h"
#include "CVehicleScannerSA.h"

#dummy
#define FUNC_IsRespondingToEvent                    0x600DB0
#define FUNC_GetCurrentEvent                        0x4ABE70
#define FUNC_GetCurrentEventType                    0x4ABE60
#define FUNC_CPedIntelligence_TestForStealthKill    0x601E00
#define FUNC_CPedIntelligence_GetTaskUseGun         0x600F70

class CPed;

class CFightManagerInterface
{
public:
    BYTE  Pad1[16];
    BYTE  UnknownState;
    BYTE  Pad2[3];
    float fStrafeState;
    float fForwardBackwardState;
};

class CPedIntelligenceSAInterface
{
public:
    // CEventHandlerHistory @ + 56
    CPedSAInterface*        pPed;
    DWORD                   taskManager;            // +4 (really CTaskManagerSAInterface)
    BYTE                    bPad[16];
    CFightManagerInterface* fightInterface;            // +24
    BYTE                    bPad2[184];
    DWORD                   vehicleScanner;            // +212 (really CVehicleScannerSAInterface)
};

class CPedIntelligenceSA : public CPedIntelligence
{
private:
    CPedIntelligenceSAInterface* internalInterface;
    CPed*                        ped;
    CTaskManagerSA*              TaskManager;
    CVehicleScannerSA*           VehicleScanner;

public:
    CPedIntelligenceSA(CPedIntelligenceSAInterface* pedIntelligenceSAInterface, CPed* ped);
    ~CPedIntelligenceSA();
    CPedIntelligenceSAInterface*  GetInterface() { return this->internalInterface; }
    bool                          IsRespondingToEvent();
    int                           GetCurrentEventType();
    CEvent*                       GetCurrentEvent();
    CTaskManager*                 GetTaskManager();
    CVehicleScanner*              GetVehicleScanner();
    bool                          TestForStealthKill(CPed* pPed, bool bUnk);
    CTaskSimpleUseGunSAInterface* GetTaskUseGun();
    CTaskSAInterface*             SetTaskDuckSecondary(unsigned short nLengthOfDuck);
};
