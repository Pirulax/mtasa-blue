/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CPerPlayerEntity.h
 *  PURPOSE:     Per-player entity linking class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include "CElement.h"
#include "packets/CPacket.h"

class CPerPlayerEntity : public CElement
{
    friend class CElement;

public:
    CPerPlayerEntity(CElement* pParent);
    ~CPerPlayerEntity();

    bool IsEntity() { return true; }
    bool IsPerPlayerEntity() { return true; };

    bool Sync(bool bSync);
    bool IsSynced() { return m_bIsSynced; };
    void SetIsSynced(bool bIsSynced) { m_bIsSynced = bIsSynced; };

    void OnReferencedSubtreeAdd(CElement* pElement);
    void OnReferencedSubtreeRemove(CElement* pElement);
    void UpdatePerPlayer();

    bool AddVisibleToReference(CElement* pElement);
    bool RemoveVisibleToReference(CElement* pElement);
    void ClearVisibleToReferences();
    bool IsVisibleToReferenced(CElement* pElement);

    bool IsVisibleToPlayer(CPlayer& Player);

    static void StaticOnPlayerDelete(CPlayer* pPlayer);
    void        OnPlayerDelete(CPlayer* pPlayer);

    void BroadcastOnlyVisible(const CPacket& Packet, bool bCheckIsSynced = false) const;
protected:
    virtual void CreateEntity(CPlayer* pPlayer);
    virtual void DestroyEntity(CPlayer* pPlayer);
private:
    void RemoveIdenticalEntries(std::set<class CPlayer*>& List1, std::set<class CPlayer*>& List2);

    void AddPlayerReference(class CPlayer* pPlayer);
    void RemovePlayerReference(class CPlayer* pPlayer);

    // Member variables
protected:
    bool m_bIsSynced = false;

    list<CElement*> m_ElementReferences;
private:
    std::set<CPlayer*> m_PlayersAdded;
    std::set<CPlayer*> m_PlayersRemoved;

    CSendList m_VisibleTo;
};
