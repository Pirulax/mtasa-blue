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

#define MAPEVENT_MAX_LENGTH_NAME 100

#include <string>

class CMapEvent
{
public:
    CMapEvent(class CLuaMain* pMain, std::string_view name, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
        float fPriorityMod);
    CMapEvent() = default;

    ~CMapEvent() = default;

    CMapEvent(const CMapEvent&) = default;
    CMapEvent(CMapEvent&&) = default;

    class CLuaMain* GetVM() const { return m_pMain; };
    CLuaFunctionRef GetLuaFunction() const { return m_iLuaFunction; };
    bool            IsPropagated() const { return m_bPropagated; }

    bool            ShouldAllowAspectRatioAdjustment() const { return m_bAllowAspectRatioAdjustment; }
    bool            ShouldForceAspectRatioAdjustment() const { return m_bForceAspectRatioAdjustment; }

    bool            ShouldBeSkipped() const { return m_bShouldBeSkipped; }
    void            SetShouldBeSkipped(bool bSkip) { m_bShouldBeSkipped = bSkip; }

    void            Call(const class CLuaArguments& Arguments);

    bool operator>(const CMapEvent& rhs) const
    { return m_eventPriority > rhs.m_eventPriority || (m_eventPriority == rhs.m_eventPriority && m_fPriorityMod > rhs.m_fPriorityMod); }
    bool operator<(const CMapEvent& rhs) const { return !(*this > rhs); }

    CMapEvent& operator=(const CMapEvent&) = default;
    CMapEvent& operator=(CMapEvent&&) = default;

private:
    class CLuaMain* m_pMain = nullptr;
    CLuaFunctionRef m_iLuaFunction = {};

    bool m_bPropagated = false;

    bool m_bShouldBeSkipped = false;

    EEventPriorityType m_eventPriority = EEventPriorityType::LOW;
    float              m_fPriorityMod = 0.0f;

    bool m_bAllowAspectRatioAdjustment = false;
    bool m_bForceAspectRatioAdjustment = false;
};
