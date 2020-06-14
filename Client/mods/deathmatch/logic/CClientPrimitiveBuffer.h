/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/mods/deathmatch/logic/CClientPrimitiveBuffer.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once
#include "CClientPrimitiveBufferInterface.h"
#include "CClientEntity.h"

enum ePrimitiveFlag : int
{
    XYZ         = 0b100000,
    XY          = 0b010000,
    UV          = 0b001000,
    DIFFUSE     = 0b000100,
    INDICES16   = 0b000010,
    INDICES32   = 0b000001,
};
DECLARE_ENUM_CLASS(ePrimitiveFlag);

constexpr static const ePrimitiveFlag s_arrPrimitiveFlags[]
{
    ePrimitiveFlag::XYZ,  ePrimitiveFlag::XY,  ePrimitiveFlag::UV,  ePrimitiveFlag::DIFFUSE,  ePrimitiveFlag::INDICES16,  ePrimitiveFlag::INDICES32 
};

static std::map<ePrimitiveFlag, size_t> s_flagToTableSizeMap // Flag <=> Vertex table size in lua 
{
    {ePrimitiveFlag::XYZ, 3},
    {ePrimitiveFlag::XY, 2},
    {ePrimitiveFlag::UV, 2},
    {ePrimitiveFlag::DIFFUSE, 1},
};

class CClientPrimitiveBuffer : public CClientPrimitiveBufferInterface, public CClientEntity
{
    DECLARE_CLASS(CClientPrimitiveBuffer, CClientEntity);

public:
    ~CClientPrimitiveBuffer();
    CClientPrimitiveBuffer(class CClientManager* pManager, ElementID ID);
    virtual eClientEntityType GetType() const { return CCLIENTPRIMITIVEBUFFER; }

    // Sorta a hack that these are required by CClientEntity...
    void Unlink();
    void GetPosition(CVector& vecPosition) const {};
    void SetPosition(const CVector& vecPosition){};

    // Supports conversion from uint32_t to uint16_t with is16BitIndecies = true
    void CreateIndexBuffer(std::vector<uint32_t>& vecIndexList, bool use16BitIndices = false)
    {
        // Release previous buffer(if any)
        SAFE_RELEASE(m_pIndexBuffer);

        const size_t memBlockSizeBytes = vecIndexList.size() * (use16BitIndices ? 2 : 4);
        assert(SUCCEEDED(m_pDevice->CreateIndexBuffer(memBlockSizeBytes, 0, use16BitIndices ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_pIndexBuffer, NULL)));

        m_uiIndicesCount = vecIndexList.size();
        m_MemoryUsageInBytes += memBlockSizeBytes;

        // Fill d3d9 index buffer
      
        // Lock and obtain pinter
        void* pVoid;
        m_pIndexBuffer->Lock(0, 0, &pVoid, 0);
        
        if (use16BitIndices)
        {
            // Convert int32_t to int16_t

            const auto arr = static_cast<uint16_t*>(pVoid);

            const auto indeciesCount = vecIndexList.size(); // Helps the compiler optimizing the loop
            for (auto i = 0; i < indeciesCount; i++)
                arr[i] = static_cast<uint16_t>(vecIndexList[i]);
        }
        else
            memcpy(pVoid, vecIndexList.data(), memBlockSizeBytes);

        m_pIndexBuffer->Unlock();
    }

    template <typename T>
    void AddVertexBuffer(std::vector<T>& vecVertexList, ePrimitiveFlag primitiveData);


    void PreDraw();
    void Draw(PrimitiveBufferSettings& settings);
    void SetPrimitiveType(D3DPRIMITIVETYPE ePrimitiveType) { m_ePrimitiveType = ePrimitiveType; }
    void SetFVF(int FVF) { m_FVF = FVF; }
    bool IsRequireMaterial() const { return m_bRequireMaterial; }
    void SetRequireMaterial(bool bRequire) { m_bRequireMaterial = bRequire; }
    void Finalize();

private:
    IDirect3DDevice9*                                 m_pDevice = nullptr;

    IDirect3DIndexBuffer9*                            m_pIndexBuffer = nullptr;

    IDirect3DVertexBuffer9*                           m_arrayVertexBuffer[8] = {nullptr};
    int                                               m_iStrideSize[8] = {0};

    LPDIRECT3DVERTEXDECLARATION9                      m_pVertexDeclaration = nullptr;

    D3DPRIMITIVETYPE                                  m_ePrimitiveType = D3DPT_TRIANGLELIST;
    std::vector<D3DVERTEXELEMENT9>                    m_vecVertexElements;

    uint32_t                                          m_uiFaceCount = 0;
    uint32_t                                          m_uiIndicesCount = 0;
    uint32_t                                          m_uiVertexCount = 0;

    int                                               m_FVF = 0;
    bool                                              m_bRequireMaterial = false;
    size_t                                            m_MemoryUsageInBytes = 0;
};

template <typename T>
void CClientPrimitiveBuffer::AddVertexBuffer(std::vector<T>& vecVertexList, ePrimitiveFlag type)
{
    size_t vertexBufferTypeIndex = -1;
    int FVF = 0;

    /*
    reminder: m_vecVertexElements is std::vector<D3DVERTEXELEMENT9>
    struct
    {
        WORD    Stream;     // Stream index
        WORD    Offset;     // Offset in the stream in bytes
        BYTE    Type;       // Data type
        BYTE    Method;     // Processing method
        BYTE    Usage;      // Semantics
        BYTE    UsageIndex; // Semantic index
    } D3DVERTEXELEMENT9, *LPD3DVERTEXELEMENT9;
    */

    switch (type)
    {
    case ePrimitiveFlag::XYZ:
    {
        vertexBufferTypeIndex = 0;
        FVF = D3DFVF_XYZ;
        m_vecVertexElements.emplace_back((WORD)index, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0);
        break;
    }
    case ePrimitiveFlag::UV:
    {
        vertexBufferTypeIndex = 1;
        FVF = D3DFVF_TEX1;
        m_vecVertexElements.emplace_back((WORD)index, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0);
        break;
    }
    case ePrimitiveFlag::DIFFUSE:
    {
        vertexBufferTypeIndex = 2;
        FVF = D3DFVF_DIFFUSE;
        m_vecVertexElements.emplace_back((WORD)index, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0);
        break;
    }
    default:
        dassert(0);
    }

    //dassert(vecVertexList.size() % 3 == 0); // not sure if this is neccessary
    m_uiFaceCount = vecVertexList.size() / 3;
    m_iStrideSize[index]  = sizeof(T);
    m_MemoryUsageInBytes += m_uiFaceCount * sizeof(T);

    IDirect3DVertexBuffer9* vertexBuffer = m_arrayVertexBuffer[vertexBufferTypeIndex];
    SAFE_DELETE(vertexBuffer); // Delete previous buffer

    // Create d3d9 vertex buffer
    m_pDevice->CreateVertexBuffer(vecVertexList.size() * sizeof(T), D3DUSAGE_WRITEONLY, FVF, D3DPOOL_MANAGED, &vertexBuffer, NULL);

    // Copy data into d3d9 buffer
    {
        void* data;

        vertexBuffer->Lock(0, 0, &data, 0);
        memcpy(pVoid, vecVertexList.data(), vecVertexList.size() * sizeof(T));
        vertexBuffer->Unlock();
    }
}
