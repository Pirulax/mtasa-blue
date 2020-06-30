/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CMapEvent.cpp
 *  PURPOSE:     Map event class
 *
 *****************************************************************************/

#include <StdInc.h>

CMapEvent::CMapEvent(CLuaMain* pMain, std::string_view name, const CLuaFunctionRef& iLuaFunction, bool bPropagated, EEventPriorityType eventPriority,
                     float fPriorityMod) :
    // Init
    m_pMain(pMain),
    m_iLuaFunction(iLuaFunction),
    m_bPropagated(bPropagated),
    m_eventPriority(eventPriority),
    m_fPriorityMod(fPriorityMod),

    // Only allow dxSetAspectRatioAdjustmentEnabled during these events
    m_bAllowAspectRatioAdjustment((name == "onClientRender") || (name == "onClientPreRender") || (name == "onClientHUDRender")),

    // Force aspect ratio adjustment for 'customblips' resource
    m_bForceAspectRatioAdjustment(m_bAllowAspectRatioAdjustment && (pMain->GetScriptName() == std::string_view("customblips")))
{
}

void CMapEvent::Call(const CLuaArguments& Arguments)
{
    if (m_pMain)
    {
        // Call our function with the given arguments
        Arguments.Call(m_pMain, m_iLuaFunction);
    }
}
