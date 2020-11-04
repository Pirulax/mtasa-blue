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
class CSendList;

#pragma once

#include <vector>
#include <unordered_map>

class CPlayer;

class CSendList
{
public:
    void Insert(CPlayer* pPlayer);

    bool Contains(CPlayer* pPlayer) const
    {
        if (const auto group = GetAPlayersGroup(pPlayer))
            return std::find(group->begin(), group->end(), pPlayer) != group->end();
        return false;
    }

    bool Erase(CPlayer* pPlayer) noexcept
    {
        if (const auto group = GetAPlayersGroup(pPlayer))
        {
            if (const auto it = std::find(group->begin(), group->end(), pPlayer); it != group->end())
            {
                group->erase(it);
                return true;
            }
        }
        return false;
    }

    const std::vector<CPlayer*>* GetPlayerGroupByBitStream(unsigned short bsver) const
    {
        if (const auto it = m_cont.find(bsver); it != m_cont.end())
            return &it->second;
        return nullptr;
    }
    std::vector<CPlayer*>* GetPlayerGroupByBitStream(unsigned short bsver) 
    { 
        // As seen in Scott Mayers book.. Thanks stackoverflow!
        return const_cast<std::vector<CPlayer*>*>(const_cast<const CSendList*>(this)->GetPlayerGroupByBitStream(bsver));
    }

    std::vector<CPlayer*>* GetAPlayersGroup(CPlayer* pPlayer);
    const std::vector<CPlayer*>* GetAPlayersGroup(CPlayer* pPlayer) const;

    // Wrappers around internal container...
    decltype(auto) begin() const { return m_cont.begin(); }
    decltype(auto) end() const { return m_cont.end(); }

    decltype(auto) begin() { return m_cont.begin(); }
    decltype(auto) end() { return m_cont.end(); }

    bool Empty() const noexcept { return m_cont.empty(); }
    bool Empty() noexcept { return m_cont.empty(); }

protected:
    std::unordered_map<unsigned short, std::vector<CPlayer*>> m_cont;
};
