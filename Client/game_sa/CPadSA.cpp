/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CPadSA.cpp
 *  PURPOSE:     Controller pad input logic
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#include "StdInc.h"

CControllerState* CPadSA::GetCurrentControllerState(CControllerState* ControllerState)
{
    DEBUG_TRACE("CControllerState * CPadSA::GetCurrentControllerState(CControllerState * ControllerState)");
    MemCpyFast(ControllerState, &this->internalInterface->NewState, sizeof(CControllerState));
    return ControllerState;
}

CControllerState* CPadSA::GetLastControllerState(CControllerState* ControllerState)
{
    DEBUG_TRACE("CControllerState * CPadSA::GetLastControllerState(CControllerState * ControllerState)");
    MemCpyFast(ControllerState, &this->internalInterface->OldState, sizeof(CControllerState));
    return ControllerState;
}

VOID CPadSA::SetCurrentControllerState(CControllerState* ControllerState)
{
    DEBUG_TRACE("VOID CPadSA::SetCurrentControllerState(CControllerState * ControllerState)");
    MemCpyFast(&this->internalInterface->NewState, ControllerState, sizeof(CControllerState));
}

VOID CPadSA::SetLastControllerState(CControllerState* ControllerState)
{
    DEBUG_TRACE("VOID CPadSA::SetLastControllerState(CControllerState * ControllerState)");
    MemCpyFast(&this->internalInterface->OldState, ControllerState, sizeof(CControllerState));
}

VOID CPadSA::Store()
{
    DEBUG_TRACE("VOID CPadSA::Store()");
    MemCpyFast(&this->StoredPad, this->internalInterface, sizeof(CPadSAInterface));
}

VOID CPadSA::Restore()
{
    DEBUG_TRACE("VOID CPadSA::Restore()");
    MemCpyFast(this->internalInterface, &this->StoredPad, sizeof(CPadSAInterface));
}

bool CPadSA::IsEnabled()
{
    bool bEnabled = *(BYTE*)FUNC_CPad_UpdatePads == 0x56;
    return bEnabled;
}

VOID CPadSA::Disable(bool bDisable)
{
    if (bDisable)
        MemPut<BYTE>(FUNC_CPad_UpdatePads, 0xC3);
    else
        MemPut<BYTE>(FUNC_CPad_UpdatePads, 0x56);

    // this->internalInterface->DisablePlayerControls = bDisable;
}

VOID CPadSA::Clear()
{
    CControllerState cs;            // create a null controller (class is inited to null)
    SetCurrentControllerState(&cs);
    SetLastControllerState(&cs);
}

VOID CPadSA::SetHornHistoryValue(bool value)
{
    internalInterface->iCurrHornHistory++;
    if (internalInterface->iCurrHornHistory >= MAX_HORN_HISTORY)
        internalInterface->iCurrHornHistory = 0;
    internalInterface->bHornHistory[internalInterface->iCurrHornHistory] = value;
}

long CPadSA::GetAverageWeapon()
{
    return internalInterface->AverageWeapon;
}

void CPadSA::SetLastTimeTouched(DWORD dwTime)
{
    internalInterface->LastTimeTouched = dwTime;
}
