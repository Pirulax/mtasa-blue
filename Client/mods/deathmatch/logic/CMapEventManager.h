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

class  CLuaFunctionRef;
class  CLuaMain;
class  CClientEntity;
struct lua_State;

class CMapEventManager
{
private:
    

public:
    ~CMapEventManager() { assert(!IsIterating()); }

    bool Add(CLuaMain* pLuaMain, const std::string& eventName, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
             float fPriorityMod);
    bool Delete(CLuaMain* pLuaMain, std::string_view name, const CLuaFunctionRef& iLuaFunction);
    bool Delete(CLuaMain* pLuaMain);
    void DeleteAll() { m_EventsMap.clear(); }
    bool HandleExists(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction) const;
    bool HasEvents() const { return !m_EventsMap.empty(); }
    void GetHandles(CLuaMain* pLuaMain, std::string_view eventName, lua_State* luaVM) const;

    bool Call(std::string_view eventName, const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis);
private:
    // Types for m_EventsMap access

    // std::vector is sorted by event priority (highest at the beginning)
    // the same priorities are sorted by insertion time(most recent is the first)
    using EventsMap = std::map<std::string, CMapEventHandlerManager, std::less<>>;
    using EventsIter = EventsMap::iterator;
private:
    inline bool IsIterating() const { return m_currIterEvent != m_EventsMap.end(); }
    inline const std::string& GetIteratedEventName() const { return m_currIterEvent->first; } // ACHTUNG: Only call if IsIterating() is true!

    EventsMap  m_EventsMap;
    EventsIter m_currIterEvent = m_EventsMap.end(); // If not iterating anything the value is m_EventsMap.end()
    size_t     m_currIterHandlerIndex = -1; // Contains the currently processed CMapEvent's index the the matching std::vector

    bool m_deleteCurrentHandlerAfterFinished = false;
};
