/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CRenderItem.cpp
 *  PURPOSE:
 *
 *****************************************************************************/

#include "StdInc.h"

 //
 // VERY IMPORTANT:
 //
 // This class might be constructed on a thread other than the main one.
 // Please make sure all calls called functions (to the main thread)
 // Are thread safe!!
 //
 //

////////////////////////////////////////////////////////////////
//
// CRenderItem::PostConstruct
//
//
////////////////////////////////////////////////////////////////
void CRenderItem::PostConstruct(CRenderItemManager* pManager, bool bIncludeInMemoryStats)
{
    m_pManager = pManager;
    m_pDevice = pManager->m_pDevice;
    m_iRefCount = 1;
    m_bIncludeInMemoryStats = bIncludeInMemoryStats;
    m_pManager->NotifyContructRenderItem(this);
}

////////////////////////////////////////////////////////////////
//
// CRenderItem::~CRenderItem
//
//
//
////////////////////////////////////////////////////////////////
CRenderItem::~CRenderItem()
{
    assert(m_iRefCount == 0);
}

////////////////////////////////////////////////////////////////
//
// CRenderItem::PreDestruct
//
//
//
////////////////////////////////////////////////////////////////
void CRenderItem::PreDestruct()
{
    assert(m_iRefCount == 0);
    m_pManager->NotifyDestructRenderItem(this);
}

////////////////////////////////////////////////////////////////
//
// CRenderItem::Release
//
//
//
////////////////////////////////////////////////////////////////
void CRenderItem::Release()
{
    assert(m_iRefCount > 0);
    if (--m_iRefCount > 0)
        return;

    PreDestruct();
    delete this;
}

////////////////////////////////////////////////////////////////
//
// CRenderItem::AddRef
//
//
//
////////////////////////////////////////////////////////////////
void CRenderItem::AddRef()
{
    assert(m_iRefCount > 0);
    ++m_iRefCount;
}
