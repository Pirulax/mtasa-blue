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

bool CSendList::Insert(CPlayer* pPlayer, bool bCheckForReInsert)
{
    const auto& group = m_cont[pPlayer->GetBitStreamVersion()];
    if (bCheckForReinsert)
        if (std::find(group->begin(), group->end(), pPlayer) != group->end())
            return false;

    group.push_back(pPlayer);
    return true;
}

auto CSendList::GetAPlayersGroup(CPlayer* pPlayer) -> std::vector<CPlayer*>*
{
    return GetPlayerGroupByBitStream(pPlayer->GetBitStreamVersion()); 
}

auto CSendList::GetAPlayersGroup(CPlayer* pPlayer) const -> const std::vector<CPlayer*>*
{
    return GetPlayerGroupByBitStream(pPlayer->GetBitStreamVersion()); 
}
