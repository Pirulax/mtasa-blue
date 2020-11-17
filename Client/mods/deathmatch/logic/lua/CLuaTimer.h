/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaTimer.h
 *  PURPOSE:     Lua timer class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CLuaTimer;

#pragma once

// Define includes
#include "LuaCommon.h"
#include "CLuaArguments.h"

#define LUA_TIMER_MIN_INTERVAL      0


class CLuaTimer
{
    friend class CLuaTimerManager;

    enum class PulseStatus : uint8_t
    {
        OK,
        SKIPPED,                    // When Status::SKIP_ONCE
        NOT_REACHED_PULSE_INTERVAL, // When m_NextPulseTick < now
        PENDING_DELETE,             // When Status::PENDING_DELETE
        INVALID_FUNCTION            // When VERIFY_FUNCTIOn fails
    };

    enum class Status : uint8_t
    {
        SKIP_ONCE,
        PENDING_DELETE,
        REPEATING
    };
public:
    CLuaTimer(const CLuaFunctionRef& iLuaFunction, CLuaArguments Arguments, CTickCount PulseInterval, size_t Repeats);
    ~CLuaTimer();

    uint GetScriptID() const { return m_uiScriptID; }

    const SLuaDebugInfo& GetLuaDebugInfo() { return m_LuaDebugInfo; }
    void                 SetLuaDebugInfo(const SLuaDebugInfo& luaDebugInfo) { m_LuaDebugInfo = luaDebugInfo; }

    CTickCount GetNextPulseTick() const noexcept { return m_NextPulseTick; }
    CTickCount GetDelay() const noexcept { return m_PulseInterval; }

    CTickCount GetTimeLeft(CTickCount now = CTickCount::Now()) const noexcept { return m_NextPulseTick - now; }

    Status GetStatus() const noexcept { return m_status; }

    bool IsPendingDelete() const noexcept { return m_status == Status::PENDING_DELETE; }
    void SetPendingDelete();

    bool Pulse(CTickCount timeNow, class CLuaMain* pLuaMain);

    void PushDetailsToLua(lua_State* luaVM, CTickCount now = CTickCount::Now()) const noexcept;

    void Reset() noexcept
    {
        if(!IsPendingDelete())
            m_NextPulseTick = CTickCount::Now();
    }

private:
    void RemoveScriptID() noexcept;

private:
    CLuaFunctionRef m_iLuaFunction;
    CLuaArguments   m_Arguments;
    SLuaDebugInfo   m_LuaDebugInfo;
    uint            m_uiScriptID;

    size_t          m_uiRepeats;
    CTickCount      m_PulseInterval;
    CTickCount      m_NextPulseTick;

    Status          m_status;
};
