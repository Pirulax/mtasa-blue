/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CPerPlayerEntity.cpp
 *  PURPOSE:     Per-player entity linking class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

static std::unordered_set<CPerPlayerEntity*> s_AllPerPlayerEntities;

CPerPlayerEntity::CPerPlayerEntity(CElement* pParent) : CElement(pParent)
{
    s_AllPerPlayerEntities.insert(this);
    AddVisibleToReference(g_pGame->GetMapManager()->GetRootElement());
};

CPerPlayerEntity::~CPerPlayerEntity()
{
    // Unsync us from everyone

    // Unreference us from what we're referencing
    for (CElement* pElement : m_ElementReferences)
        pElement->m_ElementReferenced.remove(this);

    s_AllPerPlayerEntities.erase(this);
}

bool CPerPlayerEntity::Sync(bool bSync)
{
    // Sync state changing?
    if (bSync != m_bIsSynced)
    {
        // Create / destroy this entity at every player we're visible to
        if (bSync)
            CreateEntity(NULL);
        else
            DestroyEntity(NULL);
        m_bIsSynced = bSync;
    }

    return true;
}

void CPerPlayerEntity::UpdatePerPlayer()
{
    if (m_PlayersAdded.empty() && m_PlayersRemoved.empty())            // This check reduces cpu usage when loading large maps (due to recursion)
        return;

    // Remove entries that match in both added and removed lists
    RemoveIdenticalEntries(m_PlayersAdded, m_PlayersRemoved);

    // Delete us for every player in our deleted list
    for (CPlayer* v : m_PlayersRemoved)
        DestroyEntity(v);

    // Add us for every player in our added list
    for (CPlayer* v : m_PlayersAdded)
        CreateEntity(v);

    // Clear both lists
    m_PlayersAdded.clear();
    m_PlayersRemoved.clear();
}

// Add us to pElement's reference list, and them to ours
bool CPerPlayerEntity::AddVisibleToReference(CElement* pElement)
{
    // If he isn't already referencing this element
    if (!IsVisibleToReferenced(pElement))
    {
        // Add him to our list and add us to his list
        m_ElementReferences.push_back(pElement);
        pElement->m_ElementReferenced.push_back(this);

        // Add the reference to it and update the players
        OnReferencedSubtreeAdd(pElement);
        UpdatePerPlayerEntities();

        return true;
    }

    return false;
}

// Remove us from pElement's reference list, and from ours
bool CPerPlayerEntity::RemoveVisibleToReference(CElement* pElement)
{
    // We reference this element?
    if (IsVisibleToReferenced(pElement))
    {
        // Remove him from our list and unreference us from his list
        m_ElementReferences.remove(pElement);
        pElement->m_ElementReferenced.remove(this);

        // Update the players
        OnReferencedSubtreeRemove(pElement);
        UpdatePerPlayerEntities();

        return true;
    }

    return false;
}

void CPerPlayerEntity::ClearVisibleToReferences()
{
    // For each reference in our list
    bool bCleared = false;
    for (CElement* pElement : m_ElementReferences)
    {
        // Unreference us from them
        pElement->m_ElementReferenced.remove(this);

        // Remove them, and their children, from our visible to list
        OnReferencedSubtreeRemove(pElement);
        bCleared = true;
    }

    // Clear the list and update the players
    if (bCleared)
    {
        m_ElementReferences.clear();
        UpdatePerPlayerEntities();
    }
}

// Are we visible to a referenced element? Not a player, but an element!
bool CPerPlayerEntity::IsVisibleToReferenced(CElement* pElement)
{
    return std::find(m_ElementReferences.begin(), m_ElementReferences.end(), pElement) != m_ElementReferences.end();
}

// Return true if we're visible to the given player
bool CPerPlayerEntity::IsVisibleToPlayer(CPlayer& Player)
{
    return m_VisibleTo.contains_player(&Player);
}

// Send EntityAddPacket
// If pPlayer is specified the packet is only sent to them, otherwise BroadcastOnlyVisible
void CPerPlayerEntity::CreateEntity(CPlayer* pPlayer)
{
    // Are we visible?
    if (m_bIsSynced)
    {
        // Create the add entity packet
        CEntityAddPacket Packet;
        Packet.Add(this);

        // Send it to the player if available, if not everyone
        if (pPlayer)
        {
            // Only send it to him if we can
            if (!pPlayer->DoNotSendEntities())
            {
                // CLogger::DebugPrintf ( "Created %u (%s) for %s\n", GetID (), GetName (), pPlayer->GetNick () );
                pPlayer->Send(Packet);
            }
        }
        else
        {
            // CLogger::DebugPrintf ( "Created %u (%s) for everyone (%u)\n", GetID (), GetName (), m_Players.size () );
            BroadcastOnlyVisible(Packet);
        }
    }
}

// Send EntityRemovePacket
// If pPlayer is specified the packet is only sent to them, BroadcastOnlyVisible
void CPerPlayerEntity::DestroyEntity(CPlayer* pPlayer)
{
    // Are we visible?
    if (m_bIsSynced)
    {
        // Create the remove entity packet
        CEntityRemovePacket Packet;
        Packet.Add(this);

        // Send it to the player if available, if not everyone
        if (pPlayer)
        {
            // Only send it to him if we can
            if (!pPlayer->DoNotSendEntities())
            {
                pPlayer->Send(Packet);
                // CLogger::DebugPrintf ( "Destroyed %u (%s) for %s\n", GetID (), GetName (), pPlayer->GetNick () );
            }
        }
        else
        {
            // CLogger::DebugPrintf ( "Destroyed %u (%s) for everyone (%u)\n", GetID (), GetName (), m_Players.size () );
            BroadcastOnlyVisible(Packet);
        }
    }
}

// Broadcast a packet to players we reference (only if we're synced)
void CPerPlayerEntity::BroadcastOnlyVisible(const CPacket& Packet, bool bCheckIsSynced) const
{
    // Are we synced? (if not we're not visible to anybody)
    if (!bCheckIsSynced || m_bIsSynced)
    {
        // Send it to all players we're visible to
        CPlayerManager::Broadcast(Packet, m_VisibleTo);
    }
}

// Filter out identical entities in 2 sets
void CPerPlayerEntity::RemoveIdenticalEntries(std::set<CPlayer*>& List1, std::set<CPlayer*>& List2)
{
    std::vector<CPlayer*> dupList;

    // Make list of dups
    for (std::set<CPlayer*>::iterator it = List1.begin(); it != List1.end(); it++)
        if (MapContains(List2, *it))
            dupList.push_back(*it);

    // Remove dups from both lists
    for (std::vector<CPlayer*>::iterator it = dupList.begin(); it != dupList.end(); it++)
    {
        MapRemove(List1, *it);
        MapRemove(List2, *it);
    }
}

// Add all (including pElement if its one) player type children elements of pElement to our reference list
void CPerPlayerEntity::OnReferencedSubtreeAdd(CElement* pElement)
{
    assert(pElement);

    // Is this a player?
    if (IS_PLAYER(pElement))
    {
        // Are we not already visible to that player? Add it to the list
        CPlayer* pPlayer = static_cast<CPlayer*>(pElement);
        if (!IsVisibleToPlayer(*pPlayer))
        {
            //MapInsert(Added, pPlayer);
        }

        // Add it to our reference list
        AddPlayerReference(pPlayer);
    }

    // Call ourself on all its children elements
    for (auto itChildren = pElement->IterBegin(); itChildren != IterEnd(); itChildren++)
    {
        CElement* pElement = *itChildren;
        if (pElement->CountChildren() || IS_PLAYER(pElement))            // This check reduces cpu usage when loading large maps (due to recursion)
            OnReferencedSubtreeAdd(pElement);
    }
}

// Remove all (including pElement if its one) player type children elements of pElement to our reference list
void CPerPlayerEntity::OnReferencedSubtreeRemove(CElement* pElement)
{
    assert(pElement);

    // Is this a player?
    if (IS_PLAYER(pElement))
    {
        // Remove the reference
        CPlayer* pPlayer = static_cast<CPlayer*>(pElement);
        RemovePlayerReference(pPlayer);

        // Did we just loose the last reference to that player? Add him to the list over removed players.
        if (!IsVisibleToPlayer(*pPlayer))
        {
            //MapInsert(Removed, pPlayer);
        }
    }

    // Call ourself on all our children
    for (auto itChildren = pElement->IterBegin(); itChildren != IterEnd(); itChildren++)
    {
        CElement* pElement = *itChildren;
        if (pElement->CountChildren() || IS_PLAYER(pElement))            // This check reduces cpu usage when unloading large maps (due to recursion)
            OnReferencedSubtreeRemove(pElement);
    }
}

// Check if they actually exist, then reference to a player
void CPerPlayerEntity::AddPlayerReference(CPlayer* pPlayer)
{
    if (g_pGame->GetPlayerManager()->Exists(pPlayer))
        m_VisibleTo.push_back(pPlayer);
    else
        CLogger::ErrorPrintf("CPerPlayerEntity tried to add reference for non existing player: %08x\n", pPlayer);
}

// Remove reference to a player
void CPerPlayerEntity::RemovePlayerReference(CPlayer* pPlayer)
{
    m_VisibleTo.erase_player(pPlayer);
}

//
// Hacks to stop crash
//
void CPerPlayerEntity::StaticOnPlayerDelete(CPlayer* pPlayer)
{
    for (CPerPlayerEntity* pEntity : s_AllPerPlayerEntities)
        pEntity->OnPlayerDelete(pPlayer);
}

void CPerPlayerEntity::OnPlayerDelete(CPlayer* pPlayer)
{
    m_VisibleTo.erase_player(pPlayer);
    m_PlayersAdded.erase(pPlayer);
    m_PlayersRemoved.erase(pPlayer);
}
