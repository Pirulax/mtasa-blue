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

bool g_bAllowAspectRatioAdjustment = false;
#define MAPEVENT_MAX_NAME_LENGTH 100

/* Code readability helpers, only used here */
static CLuaArgument LuaPopGlobal(lua_State* const luaVM, const char* const name)
{
    lua_getglobal(luaVM, name);
    CLuaArgument arg(luaVM, -1);
    lua_pop(luaVM, 1);
    return arg;
}

static void LuaSetGlobalValue(lua_State* luaVM, const char* name, const CLuaArgument& value)
{
    value.Push(luaVM);
    lua_setglobal(luaVM, name);
}

static void LuaSetGlobalValue(lua_State* luaVM, const char* name, const std::string& value)
{
    lua_pushlstring(luaVM, value.data(), value.length());
    lua_setglobal(luaVM, name);
}

static void LuaSetGlobalValue(lua_State* luaVM, const char* name, CElement* value)
{
    lua_pushelement(luaVM, value); // Checks for nullptr
    lua_setglobal(luaVM, name);
}

static void LuaSetGlobalValue(lua_State* luaVM, const char* name, CResource* value)
{
    if (!value)
        return;

    lua_pushresource(luaVM, value);
    lua_setglobal(luaVM, name);
}

static void LuaSetGlobalValue(lua_State* luaVM, const char* name, nullptr_t)
{
    lua_pushnil(luaVM);
    lua_setglobal(luaVM, name);
}

bool CMapEventManager::Add(CLuaMain* pLuaMain, const std::string& eventName, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
                           float fPriorityMod)
{
    if (eventName.length() > MAPEVENT_MAX_NAME_LENGTH)
        return false;

    // Grab handlers of this event
    auto& handlers = m_EventsMap[eventName];
    handlers.reserve(4);

    // Create handler here, so we can check priorities
    CMapEvent handler(pLuaMain, eventName, iLuaFunction, bPropagated, eventPriority, fPriorityMod);

    // Find position to insert(by priority)
    // See the header for more info
    auto insertAt = handlers.begin();
    for (; insertAt != handlers.end(); insertAt++)
    {
        if (handler > *insertAt)
            break;
    }

    // Might cause a reallocation (But our deep hope is it wont't)
    const auto insertedIter = handlers.insert(insertAt, std::move(handler)); // Insert before `it`

    // Check if we're iterating thru this event's handlers
    if (IsIterating() && GetIteratedEventName() == eventName)
    {
        const size_t thisEventsIndex = std::distance(handlers.begin(), insertedIter); // Grab index where this event is inserted
        if (m_currIterHandlerIndex >= thisEventsIndex) // Were past or at this event?
            m_currIterHandlerIndex++; // Move index to point at the same handler as before
        else
            insertedIter->SetShouldBeSkipped(true); // Make sure newly inserted event doesn't get called in this iteration
    }

    return true;
}

bool CMapEventManager::Delete(CLuaMain* luaMain, std::string_view eventName, const CLuaFunctionRef& luaFunction)
{
    auto iter = m_EventsMap.find(eventName);
    if (iter == m_EventsMap.end())
        return false; // This even't doesn't exist
    auto& handlers = iter->second;

    const bool isThisEventIterated = IsIterating() && GetIteratedEventName() == eventName;

    bool hasRemovedAny = false;
    for (size_t i = 0; i < handlers.size(); i++)
    {
        const CMapEvent& event = handlers[i];

        if (event.GetLuaMain() != luaMain)
            continue;

        if (event.GetLuaFunction() != luaFunction)
            continue;

        if (isThisEventIterated)
        {
            if (m_currIterHandlerIndex > i) // Is the index past this handler?
                m_currIterHandlerIndex--; // Make sure index will point at the same handler as before deleting this

            else if (i == m_currIterHandlerIndex)
            {
                // Don't delete it now, because we're iterating thru it
                m_deleteCurrentHandlerAfterFinished = true;
                continue;
            }
            assert(m_currIterHandlerIndex >= handlers.size());
        }

        handlers.erase(handlers.begin() + i); // Note: The `event` reference might be invalid after this
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

        for (size_t i = handlers.size() - 1; i != -1; i--) // Back -> front
        {
            const CMapEvent& event = handlers[i];

            if (event.GetLuaMain() != luaMain)
                continue;

            if (isThisEventIterated)
            {
                if (m_currIterHandlerIndex > i) // Is the index past this handler?
                    m_currIterHandlerIndex--; // Make sure index will point at the same handler as before deleting this

                else if (i == m_currIterHandlerIndex)
                {
                    // Delete it after we're done iteratin thru it
                    m_deleteCurrentHandlerAfterFinished = true;
                    continue;
                }
                assert(m_currIterHandlerIndex >= handlers.size());
            }

            handlers.erase(handlers.begin() + i); // Note: The `event` reference might be invalid after this
            hasRemovedAny = true;
        }
    }
    return hasRemovedAny;
}

bool CMapEventManager::Call(std::string_view viewEventName, const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis)
{
    // Check for multi-threading slipups
    assert(IsMainThread());

    if (!HasEvents())
        return false;

    const auto iterStarTimeUs = GetTimeUs();

    m_currIterEvent = m_EventsMap.find(viewEventName);
    if (m_currIterEvent == m_EventsMap.end())
        return false; // This even't doesn't exist

    SString status;

    auto& [eventName, handlers] = *m_currIterEvent;

    bool hasCalledAny = false;
    for (m_currIterHandlerIndex = 0; m_currIterHandlerIndex < handlers.size(); m_currIterHandlerIndex++)
    {
        CMapEvent* handler = &handlers[m_currIterHandlerIndex];

        if (pSource != pThis && !handler->IsPropagated()) // Call if propagated, but this handler doesnt allow such?
            continue;

        if (handler->ShouldBeSkipped()) // Newly inserted, and should be skipped?
        {
            handler->SetShouldBeSkipped(false);
            continue;
        }

        hasCalledAny = true;

        // Grab the current VM
        lua_State* const handlerLuaVM = handler->GetLuaMain()->GetVM();
        LUA_CHECKSTACK(handlerLuaVM, 1);

#if MTA_DEBUG
        const int handlerVMExpectedStackPointer = lua_gettop(handlerLuaVM);
#endif

        const auto handlerStartTimeUs = GetTimeUs();

        // Maybe allow aspect ratio?
        if (handler->ShouldAllowAspectRatioAdjustment())
        {
            g_bAllowAspectRatioAdjustment = true;
            if (handler->ShouldForceAspectRatioAdjustment()) // Maybe force it as well?
                g_pCore->GetGraphics()->SetAspectRatioAdjustmentEnabled(true);
        }

        // Maybe record event for the crash dump writer
        {
            static const bool bEnabled = g_pCore->GetDiagnosticDebug() == EDiagnosticDebug::LUA_TRACE_0000;
            if (bEnabled)
                g_pCore->LogEvent(0, "Lua Event", handler->GetLuaMain()->GetScriptName(), eventName.c_str());
        }

        if (!g_pClientGame->GetDebugHookManager()->OnPreEventFunction(eventName.c_str(), Arguments, pSource, nullptr, handler))
            continue;

        // Save the current values of globals in the handler's VM
        const auto oldSource = LuaPopGlobal(handlerLuaVM, "source");
        const auto oldThis = LuaPopGlobal(handlerLuaVM, "this");
        const auto oldResource = LuaPopGlobal(handlerLuaVM, "sourceResource");
        const auto oldResourceRoot = LuaPopGlobal(handlerLuaVM, "sourceResourceRoot");
        const auto oldEventName = LuaPopGlobal(handlerLuaVM, "eventName");
        
        // Set the "source", "this", "sourceResource" and the "sourceResourceRoot", "eventName" globals on the handler's VM
        {
            LuaSetGlobalValue(handlerLuaVM, "source", pSource);
            LuaSetGlobalValue(handlerLuaVM, "this", pThis);

            auto luaMain = g_pClientGame->GetScriptDebugging()->GetTopLuaMain();
            auto sourceResource = luaMain ? luaMain->GetResource() : nullptr;

            LuaSetGlobalValue(handlerLuaVM, "sourceResource", sourceResource);
            LuaSetGlobalValue(handlerLuaVM, "sourceResourceRoot", sourceResource ? sourceResource->GetResourceDynamicEntity() : nullptr);

            LuaSetGlobalValue(handlerLuaVM, "eventName", eventName);
        }

        // Call the Lua handler function
        handler->Call(Arguments);

        // The call to the Lua function might have caused one or more handlers to be removed
        // Make sure the pointer remains valid
        assert(m_currIterHandlerIndex >= handlers.size());
        handler = &handlers[m_currIterHandlerIndex];

        g_pClientGame->GetDebugHookManager()->OnPostEventFunction(eventName.c_str(), Arguments, pSource, nullptr, handler);

        // Reset the globals on the handler's VM
        LuaSetGlobalValue(handlerLuaVM, "source", oldSource);
        LuaSetGlobalValue(handlerLuaVM, "this", oldThis);
        LuaSetGlobalValue(handlerLuaVM, "sourceResource", oldResource);
        LuaSetGlobalValue(handlerLuaVM, "sourceResourceRoot", oldResourceRoot);
        LuaSetGlobalValue(handlerLuaVM, "eventName", oldEventName);

        dassert(lua_gettop(handlerLuaVM) == handlerVMExpectedStackPointer);

        // Aspect ratio adjustment bodges
        if (handler->ShouldAllowAspectRatioAdjustment())
        {
            g_pCore->GetGraphics()->SetAspectRatioAdjustmentEnabled(false);
            g_bAllowAspectRatioAdjustment = false;
        }

        // Do timing and performance things
        {
            const auto deltaTimeUs = GetTimeUs() - handlerStartTimeUs;
            if (deltaTimeUs > 3000 && IS_TIMING_CHECKPOINTS())
                status += SString(" (%s %d ms)", handler->GetLuaMain()->GetScriptName(), deltaTimeUs / 1000);

            CClientPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(handler->GetLuaMain(), eventName.c_str(), deltaTimeUs);
        }

        // Maybe delete this handler from the list
        if (m_deleteCurrentHandlerAfterFinished)
        {
            m_currIterHandlerIndex--; // Make sure index will point at the current element after erasing the last handler
            handlers.erase(handlers.begin() + m_currIterHandlerIndex); // Erase last event

            m_deleteCurrentHandlerAfterFinished = false;
        }       
    }

    if (IS_TIMING_CHECKPOINTS())
    {
        const auto deltaTimeUs = GetTimeUs() - iterStarTimeUs;
        if (deltaTimeUs > 5000)
            TIMING_DETAIL(SString("CMapEventManager::Call ( %s, ... ) took %d ms ( %s )", eventName.c_str(), deltaTimeUs / 1000, status.c_str()));
        
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
        if (handler.GetLuaMain() != pLuaMain)
            continue;

        if (handler.GetLuaFunction() != iLuaFunction)
            continue;

        // Make sure this isn't the currently processed handler, and will be deleted
        if (IsIterating() && m_deleteCurrentHandlerAfterFinished && &handlers[m_currIterHandlerIndex] == &handler)
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
        if (handler.GetLuaMain() != pLuaMain)
            continue;

        // Make sure this isn't the currently processed handler, and will be deleted
        if (IsIterating() && m_deleteCurrentHandlerAfterFinished && &handlers[m_currIterHandlerIndex] == &handler)
            continue;

        lua_pushnumber(luaVM, ++luaTblIndex);
        lua_getref(luaVM, handler.GetLuaFunction().ToInt());
        lua_settable(luaVM, -3);
    }
}
