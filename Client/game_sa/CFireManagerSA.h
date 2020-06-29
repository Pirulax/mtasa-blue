/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CFireManagerSA.h
 *  PURPOSE:     Header file for fire manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include <game/CFireManager.h>
#include "Common.h"
#include "CFireSA.h"

#dummy
#define FUNC_ExtinguishPoint            0x539450

#dummy
#define FUNC_StartFire                  0x48EC30
#define FUNC_StartFire_Vec              0x539F00 // ##SA##

#dummy
#define ARRAY_CFire             (VAR_CFireCount + 4)

#dummy
#define CLASS_CFireManager      0xB71F80 //##SA##

#dummy
#define DEFAULT_FIRE_PARTICLE_SIZE      1.8

#dummy
#define MAX_FIRES               60 //##SA##

class CFireManagerSA : public CFireManager
{
private:
    CFireSA* Fires[MAX_FIRES];

public:
    // constructor
    CFireManagerSA();
    ~CFireManagerSA();

    VOID   ExtinguishPoint(CVector& vecPosition, float fRadius);
    CFire* StartFire(CEntity* entityTarget, CEntity* entityCreator, float fSize);
    CFire* StartFire(CVector& vecPosition, float fSize);
    VOID   ExtinguishAllFires();
    CFire* GetFire(DWORD ID);
    DWORD  GetFireCount();
    CFire* FindFreeFire();
    CFire* GetFire(CFireSAInterface* fire);
};
