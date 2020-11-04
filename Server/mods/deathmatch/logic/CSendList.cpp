/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  PURPOSE:     An optimized container of players grouped by
 *               bitstream version.
 *               For painless and fast DoBroadcast's.
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <CSendList.h>
#include "CPlayer.h"

void CSendList::Insert(CPlayer* pPlayer)
{
    m_cont[pPlayer->GetBitStreamVersion()].push_back(pPlayer);
}

auto CSendList::GetAPlayersGroup(CPlayer* pPlayer) -> std::vector<CPlayer*>*
{
    return GetPlayerGroupByBitStream(pPlayer->GetBitStreamVersion()); 
}

auto CSendList::GetAPlayersGroup(CPlayer* pPlayer) const -> const std::vector<CPlayer*>*
{
    return GetPlayerGroupByBitStream(pPlayer->GetBitStreamVersion()); 
}
