/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEventHandlerManager.cpp
 *  PURPOSE:     Manages event handlers: delete, add, exists, call, etc..
 *               Supports handler list modification while iterating
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CMapEventHandlerManager.h"

bool g_bAllowAspectRatioAdjustment = false;

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
    if (value)
        lua_pushresource(luaVM, value);
    else
        lua_pushnil(luaVM);
    lua_setglobal(luaVM, name);
}

bool CMapEventHandlerManager::Add(const CMapEvent& handler)
{
    // Find a place (by priority) to insert our new event
    const auto insertBeforeIter = std::find_if(m_handlers.begin(), m_handlers.end(),
        [&](const CMapEvent& other) { return handler.GetPriority() >= other.GetPriority(); }
    );

    // Might cause a reallocation (But our deep hope is it wont't)
    const auto insertedIter = m_handlers.insert(insertBeforeIter, handler);

    // Special treatment if we're iterating the list currently
    if (IsIterating())
    {
        const size_t thisEventsIndex = std::distance(m_handlers.begin(), insertedIter);
        for (auto& process : m_handlerCallProcesses)
        {
            if (process.currIterIndex >= thisEventsIndex) // Are we past or at this event?
                process.currIterIndex++; // Move index to point at the same handler as before
        }
    }

    return true;
}

void CMapEventHandlerManager::MaybeDeleteFlagged()
{
    if (IsIterating())
        return;

    if (!m_areThereHandlersToRemove)
        return;

    m_areThereHandlersToRemove = false;

    m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(), // TODO: Use std::vector::erase_if (c++20)
        [&](const CMapEvent& handler) { return handler.ShouldBeDeleted(); }
    ), m_handlers.end());
}

bool CMapEventHandlerManager::Delete(CLuaMain* luaMain, const CLuaFunctionRef* luaFunction)
{
    bool hasFlaggedAny = false;
    for (auto& handler : m_handlers)
    {
        if (handler.GetLuaMain() != luaMain)
            continue;

        if (luaFunction && handler.GetLuaFunction() != *luaFunction)
            continue;

        handler.SetShouldBeDeleted(true);
        hasFlaggedAny = true;
    }

    m_areThereHandlersToRemove = hasFlaggedAny;
    MaybeDeleteFlagged();

    return hasFlaggedAny;
}

bool CMapEventHandlerManager::HandleExists(CLuaMain* pLuaMain, const CLuaFunctionRef& iLuaFunction) const
{
    for (auto& handler : m_handlers)
    {
        if (handler.ShouldBeDeleted())
            continue;

        if (handler.GetLuaMain() != pLuaMain)
            continue;

        if (handler.GetLuaFunction() != iLuaFunction)
            continue;

        return true;
    }
    return false;
}

// Push handles onto the Lua stack
void CMapEventHandlerManager::GetHandles(CLuaMain* pLuaMain, lua_State* luaVM) const
{
    lua_Number luaTblIndex = 0;
    for (const auto& handler : m_handlers)
    {
        if (handler.ShouldBeDeleted())
            continue;

        if (handler.GetLuaMain() != pLuaMain)
            continue;

        lua_pushnumber(luaVM, ++luaTblIndex);
        lua_getref(luaVM, handler.GetLuaFunction().ToInt());
        lua_settable(luaVM, -3);
    }
}

bool CMapEventHandlerManager::Call(const CLuaArguments& Arguments, CClientEntity* pSource, CClientEntity* pThis)
{
    auto& handlerCallProcess = m_handlerCallProcesses.emplace_back();
    auto& i = handlerCallProcess.currIterIndex;

    SString status;

    const auto iterStarTimeUs = GetTimeUs();

    bool hasCalledAny = false;
    for (i = 0; i < m_handlers.size(); i++)
    {
        CMapEvent* handler = &m_handlers[i];

        if (handler->ShouldBeDeleted())
            continue;

        // Call is propagated, but the handler doesnt allow such?
        if (pSource != pThis && !handler->IsPropagated())
            continue;

        // Check if the handler got inserted after the iteration started
        if (handler->GetCreationTimepoint() > handlerCallProcess.creationTime)
            continue;

        hasCalledAny = true;

        // Grab the current VM
        lua_State* const handlerLuaVM = handler->GetLuaMain()->GetVM();
        LUA_CHECKSTACK(handlerLuaVM, 1); // Make sure we have some space left

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
                g_pCore->LogEvent(0, "Lua Event", handler->GetLuaMain()->GetScriptName(), m_eventName.c_str());
        }

        if (!g_pClientGame->GetDebugHookManager()->OnPreEventFunction(m_eventName.c_str(), Arguments, pSource, nullptr, handler))
            continue;

        // Save the current values of globals in the handler's VM
        const auto oldSource = LuaPopGlobal(handlerLuaVM, "source");
        const auto oldThis = LuaPopGlobal(handlerLuaVM, "this");
        const auto oldResource = LuaPopGlobal(handlerLuaVM, "sourceResource");
        const auto oldResourceRoot = LuaPopGlobal(handlerLuaVM, "sourceResourceRoot");
        const auto oldEventName = LuaPopGlobal(handlerLuaVM, "eventName");

        // Set the "source", "this", "sourceResource", "sourceResourceRoot", and "eventName" globals on the handler's VM
        {
            auto luaMain = g_pClientGame->GetScriptDebugging()->GetTopLuaMain();
            auto sourceResource = luaMain ? luaMain->GetResource() : nullptr;

            LuaSetGlobalValue(handlerLuaVM, "sourceResource", sourceResource);
            LuaSetGlobalValue(handlerLuaVM, "sourceResourceRoot", sourceResource ? sourceResource->GetResourceDynamicEntity() : nullptr);

            LuaSetGlobalValue(handlerLuaVM, "eventName", m_eventName);

            LuaSetGlobalValue(handlerLuaVM, "source", pSource);
            LuaSetGlobalValue(handlerLuaVM, "this", pThis);
        }

        // Call the Lua handler function
        handler->Call(Arguments);

        // The call to the Lua function might have caused one or more handlers to be removed
        // Make sure the pointer remains valid
        assert(i >= m_handlers.size());
        handler = &m_handlers[i];

        g_pClientGame->GetDebugHookManager()->OnPostEventFunction(m_eventName.c_str(), Arguments, pSource, nullptr, handler);

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
            if (IS_TIMING_CHECKPOINTS() && deltaTimeUs > 3000)
                status += SString(" (%s %d ms)", handler->GetLuaMain()->GetScriptName(), deltaTimeUs / 1000);

            CClientPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(handler->GetLuaMain(), m_eventName.c_str(), deltaTimeUs);
        }
    }

    if (IS_TIMING_CHECKPOINTS())
    {
        const auto deltaTimeUs = GetTimeUs() - iterStarTimeUs;
        if (deltaTimeUs > 5000)
            TIMING_DETAIL(SString("CMapEventManager::Call ( %s, ... ) took %d ms ( %s )", m_eventName.c_str(), deltaTimeUs / 1000, status.c_str()));

    }

    m_handlerCallProcesses.pop_back(); // Pop this handler call process
    MaybeDeleteFlagged();

    return hasCalledAny;
}
