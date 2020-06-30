/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEventManager.cpp
 *  PURPOSE:     Map event manager class
 *
 *****************************************************************************/

#include "StdInc.h"

bool g_bAllowAspectRatioAdjustment = false;

CMapEventManager::CMapEventManager()
{
}

CMapEventManager::~CMapEventManager()
{
    assert(!IsIterating()); // Make sure we weren't iterating
}

bool CMapEventManager::Add(CLuaMain* pLuaMain, const std::string& eventName, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
                           float fPriorityMod)
{
    if (eventName.length() > MAPEVENT_MAX_LENGTH_NAME)
        return false;

    auto& handlers = m_EventsMap[eventName]; // Grab handlers of this event
    handlers.reserve(4);

    // Create handler here, so we can check priorities
    CMapEvent handler(pLuaMain, eventName, iLuaFunction, bPropagated, eventPriority, fPriorityMod);

    // Find position to insert(by priority)
    auto insertAt = handlers.begin();
    for (; insertAt != handlers.end(); insertAt++)
    {
        if (handler > *insertAt)
            break; // `event` has higher priority than `it`
    }

    // Might cause a reallocation (But our deep hope is it wont't)
    const auto insertedIter = handlers.insert(insertAt, std::move(handler)); // Insert before `it`

    // Check if we're iterating thru this event's handlers
    if (IsIterating() && GetIteratedEventName() == eventName)
    {
        // Skip newly inserted event - as per old behaviour
        insertedIter->SetShouldBeSkipped(true);

        const size_t thisEventsIndex = std::distance(handlers.begin(), insertedIter); // Grab index where this event is inserted
        if (m_currIterHandlerIndex >= thisEventsIndex) // Iteration index higher than the inserted event?
            m_currIterHandlerIndex++; // Move index to point at the same handler as before
    }
}

bool CMapEventManager::Delete(CLuaMain* luaMain, std::string_view eventName, const CLuaFunctionRef& luaFunction)
{
    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist
    auto& handlers = iter->second;

    const bool isThisEventIterated = IsIterating() && GetIteratedEventName() == eventName;

    bool hasRemovedAny = false;
    for (size_t i = 0; i < handlers.size(); i++) // Use indices, as iterators get invalidated when erasing
    {
        CMapEvent& event = handlers[i];

        if (event.GetVM() != luaMain)
            continue;

        if (event.GetLuaFunction() != luaFunction)
            continue;

        if (isThisEventIterated)
        {
            if (i == m_currIterHandlerIndex) // Is it the currently iterated event?
            {
                // Don't delete it now
                m_deleteCurrHandlerAfterFinished = true;
                continue;
            }

            else if (i < m_currIterHandlerIndex)
                m_currIterHandlerIndex--; // Make sure index will point at the correct handler
        }

        handlers.erase(handlers.begin() + i); // Delete it
        i--; // Make sure in the next iteration we check the next handler
        hasRemovedAny = true;
    }

    return hasRemovedAny;
}

bool CMapEventManager::Delete(CLuaMain* luaMain)
{
    bool hasRemovedAny = false;

    for (auto& [eventName, handlers] : m_EventsMap)
    {
        const bool isThisEventIterated = IsIterating() && GetIteratedEventName() == eventName;

        for (size_t i = 0; i < handlers.size(); i++) // Use indices, as iterators get invalidated when erasing
        {
            CMapEvent& event = handlers[i];

            if (event.GetVM() != luaMain)
                continue;


            if (isThisEventIterated)
            {
                if (i == m_currIterHandlerIndex) // Is it the currently iterated event?
                {
                    // Don't delete it now
                    m_deleteCurrHandlerAfterFinished = true;
                    continue;
                }

                else if (i < m_currIterHandlerIndex)
                    m_currIterHandlerIndex--; // Make sure index will point at the correct handler
            }

            handlers.erase(handlers.begin() + i); // Delete it
            i--; // Make sure in the next iteration we check the next handler
            hasRemovedAny = true;
        }
    }
    return hasRemovedAny;
}

bool CMapEventManager::Call(std::string_view inEventName, const CLuaArguments& Arguments, class CClientEntity* pSource, class CClientEntity* pThis)
{
    if (!HasEvents())
        return false;

    TIMEUS startTimeUs = GetTimeUs();

    m_currIterEvent = m_EventsMap.find(inEventName);
    if (m_currIterEvent == m_EventsMap.end())
        return false; // This even't doesn't exist

    // Most of the time this isn't filled, so its cheaper to heap allocate it if needed
    // We're going for performance here, so every tiny bit is important
    std::unique_ptr<SString> status = nullptr;

    // Check for multi-threading slipups
    assert(IsMainThread());

    // inEventName and eventName are the same strin, but
    // eventName is std::string, thus guaranteed to be null terminated
    // so we use that
    auto& [eventName, handlers] = *m_currIterEvent;

    bool hasCalledAny = false;
    for (m_currIterHandlerIndex = 0; m_currIterHandlerIndex < handlers.size(); m_currIterHandlerIndex++)
    {
        // Maybe delete last event handler from the list
        if (m_deleteCurrHandlerAfterFinished)
        {
            m_currIterHandlerIndex--; // Make sure index will point at the current element after erasing the last handler
            handlers.erase(handlers.begin() + m_currIterHandlerIndex); // Erase last event

            m_deleteCurrHandlerAfterFinished = false;
        }

        CMapEvent* handler = &handlers[m_currIterHandlerIndex];

        // Call if propagated?
        if (pSource != pThis && ! handler->IsPropagated())
            continue;

        if (handler->ShouldBeSkipped())
        {
            handler->SetShouldBeSkipped(false);
            continue;
        }

        hasCalledAny = true;

        // Grab the current VM
        lua_State* handlerLuaVM = handler->GetVM()->GetVM();

        LUA_CHECKSTACK(handlerLuaVM, 1);            // Ensure some room

#if MTA_DEBUG
        int luaStackPointer = lua_gettop(handlerLuaVM);
#endif

        TIMEUS startTime = GetTimeUs();

        // Aspect ratio adjustment bodges
        if (handler->ShouldAllowAspectRatioAdjustment())
        {
            g_bAllowAspectRatioAdjustment = true;
            if (handler->ShouldForceAspectRatioAdjustment())
                g_pCore->GetGraphics()->SetAspectRatioAdjustmentEnabled(true);
        }

        // Record event for the crash dump writer
        static bool bEnabled = (g_pCore->GetDiagnosticDebug() == EDiagnosticDebug::LUA_TRACE_0000);
        if (bEnabled)
            g_pCore->LogEvent(0, "Lua Event",   handler->GetVM()->GetScriptName(), eventName.c_str());

        if (!g_pClientGame->GetDebugHookManager()->OnPreEventFunction(eventName.c_str(), Arguments, pSource, nullptr, handler))
            continue;

        // Store the current values of the globals
        lua_getglobal(handlerLuaVM, "source");
        CLuaArgument OldSource(handlerLuaVM, -1);
        lua_pop(handlerLuaVM, 1);

        lua_getglobal(handlerLuaVM, "this");
        CLuaArgument OldThis(handlerLuaVM, -1);
        lua_pop(handlerLuaVM, 1);

        lua_getglobal(handlerLuaVM, "sourceResource");
        CLuaArgument OldResource(handlerLuaVM, -1);
        lua_pop(handlerLuaVM, 1);

        lua_getglobal(handlerLuaVM, "sourceResourceRoot");
        CLuaArgument OldResourceRoot(handlerLuaVM, -1);
        lua_pop(handlerLuaVM, 1);

        lua_getglobal(handlerLuaVM, "eventName");
        CLuaArgument OldEventName(handlerLuaVM, -1);
        lua_pop(handlerLuaVM, 1);

        // Set the "source", "this", "sourceResource" and the "sourceResourceRoot" globals on that VM
        {
            lua_pushelement(handlerLuaVM, pSource);
            lua_setglobal(handlerLuaVM, "source");

            lua_pushelement(handlerLuaVM, pThis);
            lua_setglobal(handlerLuaVM, "this");

            CLuaMain* pLuaMain = g_pClientGame->GetScriptDebugging()->GetTopLuaMain();
            CResource* pSourceResource = pLuaMain ? pLuaMain->GetResource() : nullptr;
            if (pSourceResource)
            {
                lua_pushresource(handlerLuaVM, pSourceResource);
                lua_setglobal(handlerLuaVM, "sourceResource");

                lua_pushelement(handlerLuaVM, pSourceResource->GetResourceDynamicEntity());
                lua_setglobal(handlerLuaVM, "sourceResourceRoot");
            }
            else
            {
                lua_pushnil(handlerLuaVM);
                lua_setglobal(handlerLuaVM, "sourceResource");

                lua_pushnil(handlerLuaVM);
                lua_setglobal(handlerLuaVM, "sourceResourceRoot");
            }
        }

        lua_pushlstring(handlerLuaVM, eventName.c_str(), eventName.length());
        lua_setglobal(handlerLuaVM, "eventName");

        // Call it
        handler->Call(Arguments);

        // The called function might have removed / added handlers,
        // thus the vector might got reallocated, so get the pointer again
        handler = &handlers[m_currIterHandlerIndex];

        g_pClientGame->GetDebugHookManager()->OnPostEventFunction(eventName.c_str(), Arguments, pSource, nullptr, handler);

        // Reset the globals on that VM
        OldSource.Push(handlerLuaVM);
        lua_setglobal(handlerLuaVM, "source");

        OldThis.Push(handlerLuaVM);
        lua_setglobal(handlerLuaVM, "this");

        OldResource.Push(handlerLuaVM);
        lua_setglobal(handlerLuaVM, "sourceResource");

        OldResourceRoot.Push(handlerLuaVM);
        lua_setglobal(handlerLuaVM, "sourceResourceRoot");

        OldEventName.Push(handlerLuaVM);
        lua_setglobal(handlerLuaVM, "eventName");

        dassert(lua_gettop(handlerLuaVM) == luaStackPointer);

        // Aspect ratio adjustment bodges
        if (handler->ShouldAllowAspectRatioAdjustment())
        {
            g_pCore->GetGraphics()->SetAspectRatioAdjustmentEnabled(false);
            g_bAllowAspectRatioAdjustment = false;
        }

        TIMEUS deltaTimeUs = GetTimeUs() - startTimeUs;
        if (IS_TIMING_CHECKPOINTS() && deltaTimeUs > 3000)
        {      
            if (!status) // Make sure we have a string acutally
                status = std::make_unique<SString>();
            *status += SString(" (%s %d ms)", handler->GetVM()->GetScriptName(), deltaTimeUs / 1000);      
        }
        CClientPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(handler->GetVM(), eventName.c_str(), deltaTimeUs);
    }

    if (IS_TIMING_CHECKPOINTS())
    {
        TIMEUS deltaTimeUs = GetTimeUs() - startTimeUs;
        if (deltaTimeUs > 5000)
        {
            if (status)
                TIMING_DETAIL(SString("CMapEventManager::Call ( %s, ... ) took %d ms ( %s )", eventName.c_str(), deltaTimeUs / 1000, (*status).c_str()));
            else
                TIMING_DETAIL(SString("CMapEventManager::Call ( %s, ... ) took %d ms", eventName.c_str(), deltaTimeUs / 1000));
        }
    }

    m_currIterEvent = m_EventsMap.end();
    return hasCalledAny;
}

bool CMapEventManager::HandleExists(CLuaMain* pLuaMain, std::string_view eventName, const CLuaFunctionRef& iLuaFunction) const
{
    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist

    const auto& handlers = iter->second;
    for (const auto& handler : handlers)
    {
        if (handler.GetVM() != pLuaMain)
            continue;

        if (handler.GetLuaFunction() != iLuaFunction)
            continue;

        // Make sure this isn't the currently processed handler, and will be deleted
        if (IsIterating() && m_deleteCurrHandlerAfterFinished && &handlers[m_currIterHandlerIndex] == &handler)
            continue;

        return true;
    }
    return false;
}

void CMapEventManager::GetHandles(CLuaMain* pLuaMain, std::string_view eventName, lua_State* luaVM) const
{
    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return; // This even't doesn't exist

    lua_Number luaTblIndex = 0;
    const auto& handlers = iter->second;
    for (const auto& handler : handlers)
    {
        if (handler.GetVM() != pLuaMain)
            continue;

        // Make sure this isn't the currently processed handler, and will be deleted
        if (IsIterating() && m_deleteCurrHandlerAfterFinished && &handlers[m_currIterHandlerIndex] == &handler)
            continue;

        lua_pushnumber(luaVM, ++luaTblIndex);
        lua_getref(luaVM, handler.GetLuaFunction().ToInt());
        lua_settable(luaVM, -3);
    }
}

