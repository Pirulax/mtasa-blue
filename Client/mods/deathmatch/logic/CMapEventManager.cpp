/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEventManager.cpp
 *  PURPOSE:     Map event manager class
 *
 *****************************************************************************/

#include "lua/CLuaArgument.h"
#include "StdInc.h"

#define MAPEVENT_MAX_NAME_LENGTH 100

bool CMapEventManager::Add(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
                           float fPriorityMod)
{
    if (eventName.length() > MAPEVENT_MAX_NAME_LENGTH)
        return false;

    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        iter = m_EventsMap.emplace(eventName, eventName).first;
    return iter->second.Add({ pLuaMain, eventName, iLuaFunction, bPropagated, {eventPriority, fPriorityMod} });
}

bool CMapEventManager::Delete(CLuaMain* luaMain, std::string_view eventName, const CLuaFunctionRef& luaFunction)
{
    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist
    return iter->second.Delete(luaMain, &luaFunction);
}

bool CMapEventManager::Delete(CLuaMain* luaMain)
{
    bool hasRemovedAny = false;
    for (auto& [eventName, handlersMgr] : m_EventsMap)
        hasRemovedAny |= handlersMgr.Delete(luaMain);
    return hasRemovedAny;
}

bool CMapEventManager::Call(std::string_view eventName, const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis)
{
    // Check for multi-threading slipups
    assert(IsMainThread());

    if (!HasEvents())
        return false;

    const auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist
    return iter->second.Call(Arguments, pSource, pThis);
}

bool CMapEventManager::HandleExists(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction) const
{
    const auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist
    return iter->second.HandleExists(pLuaMain, iLuaFunction);
}

void CMapEventManager::GetHandles(CLuaMain* pLuaMain, std::string_view eventName, lua_State* luaVM) const
{
    const auto iter = m_EventsMap.find(eventName);
    if (iter != m_EventsMap.end())
        iter->second.GetHandles(pLuaMain, luaVM);
}
