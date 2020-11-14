/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CRPCFunctions.cpp
 *  PURPOSE:     Remote procedure call functionality class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "net/SyncStructures.h"
#include "SharedUtil.Template.h"

extern CGame*  g_pGame;


template<CRPCFunctions::RPCFunction>
void ProcessRPC(NetBitStreamInterface& bitStream, CPlayer* pSource);

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::PLAYER_INGAME_NOTICE>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "PlayerInGameNotice");
    // Already ingame? Protocol error
    if (pSource->IsJoined())
    {
        DisconnectPlayer(g_pGame, *pSource, "Protocol error: Already ingame");
    }
    else
    {
        // Join him to the game
        g_pGame->JoinPlayer(*pSource);
    }
    UNCLOCK("NetServerPulse::RPC", "PlayerInGameNotice");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::INITIAL_DATA_STREAM>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "InitialDataStream");
    // Already sent initial stuff? Protocol error
    if (pSource->IsJoined())
    {
        DisconnectPlayer(g_pGame, *pSource, "Protocol error: Already joined");
    }
    else
    {
        // Send him the initial stuff
        g_pGame->InitialDataStream(*pSource);
    }
    UNCLOCK("NetServerPulse::RPC", "InitialDataStream");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::PLAYER_TARGET>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "PlayerTarget");
    if (pSource->IsJoined())
    {
        ElementID TargetID;
        bitStream.Read(TargetID);

        CElement* pTarget = NULL;
        if (TargetID != INVALID_ELEMENT_ID)
            pTarget = CElementIDs::GetElement(TargetID);
        pSource->SetTargetedElement(pTarget);

        // Call our script event
        CLuaArguments Arguments;
        if (pTarget)
            Arguments.PushElement(pTarget);
        else
            Arguments.PushBoolean(false);

        pSource->CallEvent("onPlayerTarget", Arguments);
    }
    UNCLOCK("NetServerPulse::RPC", "PlayerTarget");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::PLAYER_WEAPON>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "PlayerWeapon");
    if (pSource->IsJoined() && pSource->IsSpawned())
    {
        unsigned char ucPrevSlot = pSource->GetWeaponSlot();

        // We don't get the puresync packet containing totalAmmo = 0 for slot 8 (THROWN), slot 7 (HEAVY) and slot 9 (SPECIAL)
        if ((bitStream.Version() >= 0x44 && ucPrevSlot == WEAPONSLOT_TYPE_THROWN) || bitStream.Version() >= 0x4D)
        {
            if (bitStream.ReadBit() && (ucPrevSlot == WEAPONSLOT_TYPE_THROWN ||
                (bitStream.Version() >= 0x5A && (ucPrevSlot == WEAPONSLOT_TYPE_HEAVY || ucPrevSlot == WEAPONSLOT_TYPE_SPECIAL))))
            {
                CWeapon* pPrevWeapon = pSource->GetWeapon(ucPrevSlot);
                pPrevWeapon->usAmmo = 0;
                pPrevWeapon->usAmmoInClip = 0;
            }
        }

        SWeaponSlotSync slot;
        bitStream.Read(&slot);
        unsigned int uiSlot = slot.data.uiSlot;

        if (uiSlot != ucPrevSlot)
        {
            CLuaArguments Arguments;
            Arguments.PushNumber(pSource->GetWeaponType(ucPrevSlot));
            Arguments.PushNumber(pSource->GetWeaponType(uiSlot));

            pSource->CallEvent("onPlayerWeaponSwitch", Arguments);
        }

        pSource->SetWeaponSlot(uiSlot);
        CWeapon* pWeapon = pSource->GetWeapon(uiSlot);

        if (CWeaponNames::DoesSlotHaveAmmo(uiSlot))
        {
            if (pWeapon)
            {
                SWeaponAmmoSync ammo(pWeapon->ucType, true, true);
                bitStream.Read(&ammo);

                pWeapon->usAmmo = ammo.data.usTotalAmmo;
                pWeapon->usAmmoInClip = ammo.data.usAmmoInClip;
            }
        }
        else if (pWeapon)
        {
            pWeapon->usAmmo = 1;
            pWeapon->usAmmoInClip = 1;
            // Keep the server synced with the client (GTASA gives the client a detonator when they shoot so if they changed to slot 12 they obviously have one)
            if (uiSlot == 12)
                // Give them the detonator
                CStaticFunctionDefinitions::GiveWeapon(pSource, 40, 1, true);
        }
    }
    UNCLOCK("NetServerPulse::RPC", "PlayerWeapon");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::KEY_BIND>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "KeyBind");

    unsigned char ucType;
    bool          bHitState = false;
    if (bitStream.ReadBit() == true)
        ucType = 1;
    else
        ucType = 0;
    bitStream.ReadBit(bHitState);

    unsigned char ucKeyLength = bitStream.GetNumberOfUnreadBits() >> 3;

    char szKey[256];
    bitStream.Read(szKey, ucKeyLength);
    szKey[ucKeyLength] = 0;

    pSource->GetKeyBinds()->ProcessKey(szKey, bHitState, (eKeyBindType)ucType);

    UNCLOCK("NetServerPulse::RPC", "KeyBind");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::CURSOR_EVENT>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "CursorEvent");

    SMouseButtonSync button;
    unsigned short   usX;
    unsigned short   usY;
    SPositionSync    position(false);
    bool             bHasCollisionElement;
    ElementID        elementID;

    if (bitStream.Read(&button) && bitStream.ReadCompressed(usX) && bitStream.ReadCompressed(usY) && bitStream.Read(&position) &&
        bitStream.ReadBit(bHasCollisionElement) && (!bHasCollisionElement || bitStream.Read(elementID)))
    {
        unsigned char ucButton = button.data.ucButton;
        CVector2D     vecCursorPosition(static_cast<float>(usX), static_cast<float>(usY));
        CVector       vecPosition = position.data.vecPosition;
        if (!bHasCollisionElement)
            elementID = INVALID_ELEMENT_ID;

        if (pSource->IsJoined())
        {
            // Get the button and state
            const char* szButton = NULL;
            const char* szState = NULL;
            switch (ucButton)
            {
            case 0:
                szButton = "left";
                szState = "down";
                break;
            case 1:
                szButton = "left";
                szState = "up";
                break;
            case 2:
                szButton = "middle";
                szState = "down";
                break;
            case 3:
                szButton = "middle";
                szState = "up";
                break;
            case 4:
                szButton = "right";
                szState = "down";
                break;
            case 5:
                szButton = "right";
                szState = "up";
                break;
            }
            if (szButton && szState)
            {
                CElement* pElement = CElementIDs::GetElement(elementID);
                if (pElement)
                {
                    // Call the onElementClicked event
                    CLuaArguments Arguments;
                    Arguments.PushString(szButton);
                    Arguments.PushString(szState);
                    Arguments.PushElement(pSource);
                    Arguments.PushNumber(vecPosition.fX);
                    Arguments.PushNumber(vecPosition.fY);
                    Arguments.PushNumber(vecPosition.fZ);
                    pElement->CallEvent("onElementClicked", Arguments);
                }
                // Call the onPlayerClick event
                CLuaArguments Arguments;
                Arguments.PushString(szButton);
                Arguments.PushString(szState);
                if (pElement)
                    Arguments.PushElement(pElement);
                else
                    Arguments.PushNil();
                Arguments.PushNumber(vecPosition.fX);
                Arguments.PushNumber(vecPosition.fY);
                Arguments.PushNumber(vecPosition.fZ);
                Arguments.PushNumber(vecCursorPosition.fX);
                Arguments.PushNumber(vecCursorPosition.fY);
                pSource->CallEvent("onPlayerClick", Arguments);

                // TODO: iterate server-side element managers for the click events, eg: colshapes
            }
        }
    }
    UNCLOCK("NetServerPulse::RPC", "CursorEvent");
}

template<>
void ProcessRPC<CRPCFunctions::RPCFunction::REQUEST_STEALTH_KILL>(NetBitStreamInterface& bitStream, CPlayer* pSource)
{
    CLOCK("NetServerPulse::RPC", "RequestStealthKill");

    ElementID ID;
    bitStream.Read(ID);

    CPed* pTargetPed = dynamic_cast<CPed*>(CElementIDs::GetElement(ID));
    if (!pTargetPed)
        return; 

    if (pSource->IsDead() || pTargetPed->IsDead())
        return;  // They should be dead

     // Do we have any record of the killer currently having a knife?
    if (pSource->GetWeaponType(1) == 4)
    {
        // Are they close enough?
        if (DistanceBetweenPoints3D(pSource->GetPosition(), pTargetPed->GetPosition()) <= STEALTH_KILL_RANGE)
        {
            CLuaArguments Arguments;
            Arguments.PushElement(pTargetPed);
            if (pSource->CallEvent("onPlayerStealthKill", Arguments))
            {
                // Start the stealth kill
                CStaticFunctionDefinitions::KillPed(pTargetPed, pSource, 4 /*WEAPONTYPE_KNIFE*/, 9 /*BODYPART_HEAD*/, true);
            }
        }
    }
    else
    {
        // You shouldn't be able to get here without cheating to get a knife.
        if (!g_pGame->GetConfig()->IsDisableAC("2"))
        {
            // Kick disabled as sometimes causing false positives due weapon slot sync problems
#if 0
            CStaticFunctionDefinitions::KickPlayer(pSource, NULL, "AC #2: You were kicked from the game");
#endif
        }
    }
    UNCLOCK("NetServerPulse::RPC", "RequestStealthKill");
}

// Must be at the bottom otherwise the template magic refuses to work
void CRPCFunctions::ProcessPacket(const NetServerPlayerID& Socket, NetBitStreamInterface& bitStream)
{
    CPlayer* pSource = g_pGame->GetPlayerManager()->Get(Socket);
    if (!pSource || pSource->IsBeingDeleted())
        return;

    // Read function ID as a unsigned char
    RPCFunction rpcfn;
    bitStream.Read(reinterpret_cast<unsigned char&>(rpcfn));

    EnumToFunctionDispatch(rpcfn,
        [](auto rpcfn) { return &ProcessRPC<(RPCFunction)rpcfn>; }, bitStream, pSource);
}
