/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaTimer.cpp
 *  PURPOSE:     Lua timer class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>

CLuaTimer::CLuaTimer(const CLuaFunctionRef& iLuaFunction, CLuaArguments Arguments, CTickCount PulseInterval, size_t Repeats) :
    m_uiScriptID(CIdArray::PopUniqueId(this, EIdClass::TIMER)),
    m_uiRepeats(Repeats),
    m_PulseInterval(PulseInterval),
    m_NextPulseTick(CTickCount::Now() + PulseInterval), // Default init
    m_iLuaFunction(iLuaFunction),
    m_Arguments(std::move(Arguments))
{
}

CLuaTimer::~CLuaTimer()
{
    RemoveScriptID();
}

void CLuaTimer::RemoveScriptID() noexcept
{
    if (m_uiScriptID != INVALID_ARRAY_ID)
    {
        CIdArray::PushUniqueId(this, EIdClass::TIMER, m_uiScriptID);
        m_uiScriptID = INVALID_ARRAY_ID;
    }
}

void CLuaTimer::SetPendingDelete()
{
    dassert(!IsPendingDelete());

    RemoveScriptID();
    m_status = Status::PENDING_DELETE;
}

bool CLuaTimer::Pulse(CTickCount timeNow, CLuaMain* pLuaMain)
{
    /* Checked by the manager
    if (m_status == Status::PENDING_DELETE)
        return false;
    */ 

    if (m_NextPulseTick > timeNow)
        return false;

    if (!VERIFY_FUNCTION(m_iLuaFunction))
        return false; // Perhaps `SetPendingDelete()` here?

    // Actually process this timer (call the Lua function)
    {
        // Save debug info, so its reported correctly
        g_pClientGame->GetScriptDebugging()->SaveLuaDebugInfo(GetLuaDebugInfo());

        // Grab the main state
        // TODO: Check if coroutines work properly if its like this..
        lua_State* pLuaMainState = pLuaMain->GetVM();

        // Ensure some room
        LUA_CHECKSTACK(pLuaMainState, 1);

        // Store the current value of the sourceTimer global
        lua_getglobal(pLuaMainState, "sourceTimer");
        CLuaArgument OldSource(pLuaMainState, -1);
        lua_pop(pLuaMainState, 1);

        // Set the "sourceTimer" global
        lua_pushtimer(pLuaMainState, this);
        lua_setglobal(pLuaMainState, "sourceTimer");

        // Call the handler Lua function
        m_Arguments.Call(pLuaMain, m_iLuaFunction);

        // Reset the sourceTimer global
        OldSource.Push(pLuaMainState);
        lua_setglobal(pLuaMainState, "sourceTimer");

        // Reset the debug info it
        g_pClientGame->GetScriptDebugging()->SaveLuaDebugInfo({});
    }

    if (m_uiRepeats != 0) // Not infinite repeats?
    {
        m_uiRepeats--;
        if (!m_uiRepeats)
            SetPendingDelete(); // No more repeats
    }

    if (!IsPendingDelete())
        m_NextPulseTick = timeNow + m_PulseInterval;

    return true;
}

void CLuaTimer::PushDetailsToLua(lua_State* luaVM, CTickCount now) const noexcept
{
    lua_pushnumber(luaVM, GetTimeLeft(now).ToDouble());
    lua_pushnumber(luaVM, (double)m_uiRepeats);
    lua_pushnumber(luaVM, GetDelay().ToDouble());
}
