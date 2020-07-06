/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEventHandlerManager.h
 *
 *****************************************************************************/

#pragma once

#include <vector>
#include <string_view>
#include "CMapEvent.h"

class  CLuaFunctionRef;
class  CLuaMain;
struct lua_State;

#ifdef MTA_CLIENT // When included in the .cpp along with StdInc.h file this works
class  CClientEntity;
#else
class CElement;
#endif

class CMapEventHandlerManager
{
public:
    CMapEventHandlerManager(std::string_view eventName) : m_eventName(eventName) { m_handlers.reserve(4); }
    ~CMapEventHandlerManager() { assert(!IsIterating()); }

    inline bool IsIterating() const { return !m_handlerCallProcesses.empty(); }

    bool CMapEventHandlerManager::Add(const CMapEvent& handler);

    bool Delete(CLuaMain* luaMain, const CLuaFunctionRef* luaFunction = nullptr);

    bool HandleExists(CLuaMain* luaMain, const CLuaFunctionRef& luaFunction) const;

    // Push handles onto the Lua stack
    void GetHandles(CLuaMain* luaMain, lua_State* luaVM) const;

    bool Call(const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis);

private:
    void MaybeDeleteFlagged();

private:
    // Tl;dr; there can be somewhat recursive event calls, so we must store
    // a list of iteratin indices. Usually theres only 1 process at a time.
    // This list's size will never exceed the Lua C stack size(which is 200)
    struct SHandlerCallProcess
    {
        // The index to m_handlers, we use indices here, as iterators
        // can get invalidated when we insert a new handler into m_handlers
        // while iterating, which would cause UB
        size_t currIterIndex = -1;

        // Must be stored, so older iteration processes won't call a newly inserted event
        std::chrono::steady_clock::time_point creationTime = std::chrono::steady_clock::now();
    };
private:
    std::vector<SHandlerCallProcess> m_handlerCallProcesses;

    // This down here is sroted by event priority (the higher the priority and the more recent the lower the position is)
    // Eg.: [high+1, high+1(oldest), low, low-1(most recent)]
    std::vector<CMapEvent> m_handlers;

    const std::string m_eventName;


    bool m_areThereHandlersToRemove = false;
};
