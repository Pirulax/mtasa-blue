/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaTimerManager.h
 *  PURPOSE:     Lua timer manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

class CLuaTimerManager;

#pragma once

#include "LuaCommon.h"
#include "CLuaTimer.h"
#include <forward_list>

class CLuaTimerManager
{
public:
    CLuaTimerManager() = default;
    ~CLuaTimerManager() = default;

    void DoPulse(CLuaMain* pLuaMain);

    [[nodiscard]]
    CLuaTimer* GetTimerFromScriptID(uint uiScriptID) const;

    [[nodiscard]]
    CLuaTimer*    AddTimer(const CLuaFunctionRef& iLuaFunction, CTickCount llTimeDelay, unsigned int uiRepeats, CLuaArguments Arguments);
    void          RemoveTimer(CLuaTimer* pLuaTimer);
    void          RemoveAllTimers();

    [[nodiscard]]
    unsigned long GetTimerCount() const { return 0; }

    [[nodiscard]]
    bool IsValidTimer(CLuaTimer* pLuaTimer) const noexcept;

    void ResetTimer(CLuaTimer* pLuaTimer);

private:
    using TimerList_t = std::forward_list<CLuaTimer>;
private:
    // Sorted list of timers
    // sorted by NextPulseTime
    TimerList_t m_TimerList;

    size_t m_TimerCount = 0;

    // We don't want newly inserted imters to be Pulsed
    // And because we insert timers at the end
    // we can just update this variable, and check against it
    // in the DoPulse loop, and treat it as `m_TimerList.end()`
    // std::list::insert doesn't invalidate .end(), nor does ::erase
    // so we can treat list's end() as a constant
    TimerList_t::iterator m_pulseListEnd = m_TimerList.end();

    // An iterator to the last timer in the list
    // should always be kept up-to-date
    // Not to be confused: this iterator is the past, not past-the-end (which m_TimerList.end() is)
    TimerList_t::iterator m_lastTimer = m_TimerList.end();

    bool m_bInPulse = false;
};
