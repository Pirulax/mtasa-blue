/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/logic/lua/CLuaTimerManager.cpp
 *  PURPOSE:     Lua timer manager class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>
#include <iostream>

void CLuaTimerManager::DoPulse(CLuaMain* pLuaMain)
{
    using namespace std::chrono;

    if (!m_TimerCount) // Might crash otherwise
        return;

    // Save the beginning when we've started iterating.
    // This ensures that newly inserted timers aren't 
    // processed in the same pulse they're  inserted at
    const auto lastTimerWhenPulseBegan = m_lastTimer;


    CTickCount now = CTickCount::Now();

    const auto begin = high_resolution_clock::now();

    CTimeUsMarker marker;
    marker.Set("Start");


    // TODO(C++20): Use m_TimerList.erase_if

    // Process all pending deletes at the front
    while (m_TimerCount && m_TimerList.front().IsPendingDelete())
    {
        m_TimerList.pop_front();
        m_TimerCount--;
    }


    if (m_TimerCount)
    {
        // Pulse and/or delete other timers here
        // This code intentionally doesn't check if the first timer can be deleted
        // because the above code handles already did that
        for (TimerList_t::iterator it = m_TimerList.begin(); m_TimerCount; it++)
        {
            it->Pulse(now, pLuaMain); // Pulse timers, including lastTimerWhenPulseBegan

            // Make sure timers after this one (if any) aren't processed,
            // as they've been inserted while Pulsing.
            if (it == lastTimerWhenPulseBegan)
                break;

            if (const auto next = std::next(it); next != m_TimerList.end())
            {
                // This also makes sure that the next timer (that is, the Pulse'd one)
                // wont have a pending delete, so no need to check for pending deletes
                // in `CluaTimer::Pulse`
                if (next->IsPendingDelete())
                {
                    if (next == m_lastTimer)
                        m_lastTimer = it;

                    m_TimerCount--;
                    if (m_TimerList.erase_after(it) == m_TimerList.end())
                        break; // it++ would be the end
                }
            }
            else
                break; // it++ would be the end
        }
    }
    else
        m_lastTimer = m_TimerList.end();

    marker.Set("Do pending deletes");
    
    
    const auto interval = high_resolution_clock::now() - begin;
    const auto inms = duration_cast<milliseconds>(interval);
    const auto time = inms.count();
    if (time > 1) {
        g_pCore->GetConsole()->Printf("CLuaTimerManager::DoPulse took %u ms", (unsigned)time);
        g_pCore->GetConsole()->Print(marker.GetString());
    }

    // Check if timer count is correct
    dassert(std::distance(m_TimerList.begin(), m_TimerList.end()) == m_TimerCount);
}

bool CLuaTimerManager::IsValidTimer(CLuaTimer* pLuaTimer) const noexcept
{
    if (!pLuaTimer)
        return false;

    // See if pLuaTimer is a pointer to any of the objects in the list
    const auto it = std::find_if(m_TimerList.begin(), m_TimerList.end(), [pLuaTimer](const CLuaTimer& timer) {
        return &timer == pLuaTimer;
    });

    return it != m_TimerList.end() && !it->IsPendingDelete();
}

void CLuaTimerManager::RemoveTimer(CLuaTimer* pLuaTimer)
{
    if (IsValidTimer(pLuaTimer))
        pLuaTimer->SetPendingDelete();
}

void CLuaTimerManager::RemoveAllTimers()
{
    m_TimerList.clear();
    m_TimerCount = 0;
}

void CLuaTimerManager::ResetTimer(CLuaTimer* pLuaTimer)
{
    if (IsValidTimer(pLuaTimer))
        pLuaTimer->Reset();
}

CLuaTimer* CLuaTimerManager::GetTimerFromScriptID(uint uiScriptID) const
{
    CLuaTimer* pLuaTimer = (CLuaTimer*)CIdArray::FindEntry(uiScriptID, EIdClass::TIMER);
    return IsValidTimer(pLuaTimer) ? pLuaTimer : nullptr;
}

CLuaTimer* CLuaTimerManager::AddTimer(const CLuaFunctionRef& iLuaFunction, CTickCount llTimeDelay, unsigned int uiRepeats, const CLuaArguments& Arguments)
{
    // Check for the minimum interval
#if LUA_TIMER_MIN_INTERVAL
    if (llTimeDelay.ToLongLong() < LUA_TIMER_MIN_INTERVAL)
        return nullptr;
#endif


    if (!VERIFY_FUNCTION(iLuaFunction))
        return nullptr;

    if (m_lastTimer == m_TimerList.end())
    {
        m_TimerList.emplace_front(iLuaFunction, std::move(Arguments), llTimeDelay, uiRepeats);
        m_lastTimer = m_TimerList.begin();
    }
    else
        m_lastTimer = m_TimerList.emplace_after(m_lastTimer, iLuaFunction, std::move(Arguments), llTimeDelay, uiRepeats);

    m_TimerCount++;
    return &(*m_lastTimer);
}
