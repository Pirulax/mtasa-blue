/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CRPCFunctions.h
 *  PURPOSE:     Remote procedure call functionality class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include "CCommon.h"
#include <vector>
#include <net/CNetServer.h>
#include "CPlayerManager.h"

class CPlayer;
class CGame;

class CRPCFunctions
{
public:
    static void  ProcessPacket(const NetServerPlayerID& Socket, NetBitStreamInterface& bitStream);

public:
    enum class RPCFunction
    {
        PLAYER_INGAME_NOTICE,
        INITIAL_DATA_STREAM,
        PLAYER_TARGET,
        PLAYER_WEAPON,
        KEY_BIND,
        CURSOR_EVENT,
        REQUEST_STEALTH_KILL,

        END,
        BEGIN = PLAYER_INGAME_NOTICE
    };
};
