/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CClientPrimitiveBufferManager.cpp
 *  PURPOSE:     Text PrimitiveBuffer manager class
 *
 *****************************************************************************/

class CClientPrimitiveBufferManager;

#pragma once

#include "CClientManager.h"
#include <list>

class CClientPrimitiveBuffer;

class CClientPrimitiveBufferManager
{
    friend class CClientPrimitiveBuffer;

public:
    CClientPrimitiveBufferManager(CClientManager* pManager);
    ~CClientPrimitiveBufferManager();

    CClientPrimitiveBuffer* Create();

    void Delete(CClientPrimitiveBuffer* pPrimitiveBuffer);
    void DeleteAll();

    CClientPrimitiveBuffer*        Get(unsigned long ulID);
    static CClientPrimitiveBuffer* Get(ElementID ID);

private:

    void AddToList(std::unique_ptr<CClientPrimitiveBuffer> pPrimitiveBuffer) { m_List.push_back(std::move(pPrimitiveBuffer)); };
    void RemoveFromList(CClientPrimitiveBuffer* pPrimitiveBuffer);

private:
    CClientManager* m_pManager;

    std::list<std::unique_ptr<CClientPrimitiveBuffer>> m_List;
};
