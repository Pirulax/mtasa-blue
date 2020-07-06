/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEventManager.h
 *  PURPOSE:     Map event manager class header
 *
 *****************************************************************************/

#pragma once

#include <map>
#include <vector>
#include <string_view>
#include "CMapEvent.h"
#include "CMapEventHandlerManager.h"

class  CLuaFunctionRef;
class  CLuaMain;
class  CClientEntity;
struct lua_State;

class CMapEventManager
{
public:
    bool Add(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
             float fPriorityMod);
    bool Delete(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction);
    bool Delete(CLuaMain* pLuaMain);
    void DeleteAll() { m_EventsMap.clear(); }

    bool HandleExists(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction) const;
    void GetHandles(CLuaMain* pLuaMain, std::string_view eventName, lua_State* luaVM) const;
    bool HasEvents() const { return !m_EventsMap.empty(); }

    bool Call(std::string_view eventName, const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis);

private:
    std::map<std::string, CMapEventHandlerManager, std::less<>>  m_EventsMap;
};
