/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/mods/deathmatch/logic/CClientPrimitiveBuffer.cpp
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>

enum ePrimitiveFlag;

CClientPrimitiveBuffer::CClientPrimitiveBuffer(class CClientManager* pManager, ElementID ID) : 
    ClassInit(this), 
    CClientEntity(ID),
    m_pDevice(g_pCore->GetGraphics()->GetDevice())
{
    m_pManager = pManager;
    SetTypeName("primitivebuffer");
}

void CClientPrimitiveBuffer::Unlink()
{
    g_pClientGame->GetPrimitiveBufferManager()->Delete(this);
}

CClientPrimitiveBuffer::~CClientPrimitiveBuffer()
{
    SAFE_RELEASE(m_pIndexBuffer);

    for (auto pVBuffer : m_arrayVertexBuffer)
        SAFE_RELEASE(pVBuffer)
}

void CClientPrimitiveBuffer::Finalize()
{
    m_vecVertexElements.push_back(D3DDECL_END());
    m_pDevice->CreateVertexDeclaration(&m_vecVertexElements[0], &m_pVertexDeclaration);
}

void CClientPrimitiveBuffer::PreDraw()
{
    m_pDevice->SetFVF(m_FVF);
    m_pDevice->SetVertexDeclaration(m_pVertexDeclaration);

    if (m_pIndexBuffer)
        m_pDevice->SetIndices(m_pIndexBuffer);

    for (int i = 0; i < 8; i++)
        if (m_arrayVertexBuffer[i] != nullptr)
            m_pDevice->SetStreamSource(i, m_arrayVertexBuffer[i], 0, m_iStrideSize[i]);
}

void CClientPrimitiveBuffer::Draw(PrimitiveBufferSettings& settings)
{
    // Set d3d9 transform
    float fMatrixBuffer[16];
    settings.matrix.GetBuffer(fMatrixBuffer);
    m_pDevice->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)fMatrixBuffer);

    if (m_pIndexBuffer)
        m_pDevice->DrawIndexedPrimitive(m_ePrimitiveType, 0, 0, m_uiFaceCount, 0, m_uiIndicesCount);
    else
        m_pDevice->DrawPrimitive(m_ePrimitiveType, 0, m_uiFaceCount);
}
