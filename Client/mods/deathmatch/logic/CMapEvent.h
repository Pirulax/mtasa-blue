/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEvent.h
 *  PURPOSE:     Map event class header
 *
 *****************************************************************************/

#pragma once

#include <string>
#include "lua/CLuaFunctionRef.h"
#include "lua/CLuaArgument.h"
#include "Enums.h"

class CLuaMain;

class SMapEventPriority
{
public:
    SMapEventPriority(EEventPriorityType eventPriority, float fPriorityMod) : m_eventPriority(eventPriority), m_fPriorityMod(fPriorityMod) {}

    bool operator>(const SMapEventPriority& rhs) const
    {
        return m_eventPriority > rhs.m_eventPriority || (m_eventPriority == rhs.m_eventPriority && m_fPriorityMod > rhs.m_fPriorityMod);
    }

    bool operator<(const SMapEventPriority& rhs) const { return !(*this > rhs); }
    bool operator>=(const SMapEventPriority& rhs) const { return !(*this < rhs); }
    bool operator<=(const SMapEventPriority& rhs) const { return !(*this > rhs); }

private:
    EEventPriorityType m_eventPriority = EEventPriorityType::LOW;
    float              m_fPriorityMod = 0.0f;
};

class CMapEvent
{
public:
    CMapEvent(CLuaMain* pMain, std::string_view name, const CLuaFunctionRef& iLuaFunction, bool bPropagated, SMapEventPriority priority);
    CMapEvent() noexcept = default;

    ~CMapEvent() = default;

    CMapEvent(const CMapEvent&) = default;
    CMapEvent(CMapEvent&&) noexcept = default;

    class CLuaMain* GetLuaMain() const { return m_pLuaMain; };
    CLuaFunctionRef GetLuaFunction() const { return m_iLuaFunction; };
    bool            IsPropagated() const { return m_bPropagated; }

    bool            ShouldAllowAspectRatioAdjustment() const { return m_bAllowAspectRatioAdjustment; }
    bool            ShouldForceAspectRatioAdjustment() const { return m_bForceAspectRatioAdjustment; }

    bool            ShouldBeSkipped() const { return m_bShouldBeSkipped || m_bShouldBeDeleted; }
    void            SetShouldBeSkipped(bool bSkip) { m_bShouldBeSkipped = bSkip; }

    bool            ShouldBeDeleted() const { return m_bShouldBeDeleted; }
    void            SetShouldBeDeleted(bool bDelete) { m_bShouldBeDeleted = bDelete; }

    void            Call(const CLuaArguments& Arguments) const { if (m_pLuaMain) { Arguments.Call(m_pLuaMain, m_iLuaFunction); } }


    const SMapEventPriority& GetPriority() const { return m_priority; }

    CMapEvent& operator=(const CMapEvent&) = default;
    CMapEvent& operator=(CMapEvent&&) noexcept = default;

private:
    CLuaMain*       m_pLuaMain = nullptr;
    CLuaFunctionRef m_iLuaFunction = {};

    bool m_bPropagated = false;

    bool m_bShouldBeSkipped = false;
    bool m_bShouldBeDeleted = false;

    SMapEventPriority m_priority;

    bool m_bAllowAspectRatioAdjustment = false;
    bool m_bForceAspectRatioAdjustment = false;
};
