/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CModelCacheManager.h"

namespace
{
    struct SModelCacheInfo
    {
        SModelCacheInfo() : fClosestDistSq(0), bIsModelCachedHere(false), bIsModelLoadedByGame(false) {}
        CTickCount lastNeeded;
        CTickCount firstNeeded;
        float      fClosestDistSq;
        bool       bIsModelCachedHere;
        bool       bIsModelLoadedByGame;
    };
}            // namespace

///////////////////////////////////////////////////////////////
//
//
// CModelCacheManager
//
//
///////////////////////////////////////////////////////////////
class CModelCacheManagerImpl : public CModelCacheManager
{
public:
    ZERO_ON_NEW

    // CModelCacheManager interface
    virtual void DoPulse();
    virtual void GetStats(SModelCacheStats& outStats);
    virtual bool UnloadModel(ushort usModelId);
    virtual void OnRestreamModel(ushort usModelId);
    virtual void OnClientClose();
    virtual void UpdatePedModelCaching(const std::map<ushort, float>& newNeedCacheList);
    virtual void UpdateVehicleModelCaching(const std::map<ushort, float>& newNeedCacheList);
    virtual void AddModelToPersistentCache(ushort usModelId);

    // CModelCacheManagerImpl methods
    CModelCacheManagerImpl();
    ~CModelCacheManagerImpl();

    void PreLoad();
    void RemoveCacheRefs(std::map<ushort, SModelCacheInfo>& currentCacheInfoMap);
    void UpdateModelCaching(const std::map<ushort, float>& newNeededList, std::map<ushort, SModelCacheInfo>& currentCacheInfoMap, uint uiMaxCachedAllowed);
    int  GetModelRefCount(ushort usModelId);
    void AddModelRefCount(ushort usModelId);
    void SubModelRefCount(ushort usModelId);

protected:
    CGame*                            m_pGame;
    int                               m_iFrameCounter;
    CTickCount                        m_TickCountNow;
    bool                              m_bDonePreLoad;
    uint                              m_uiMaxCachedPedModels;
    uint                              m_uiMaxCachedVehicleModels;
    std::map<ushort, SModelCacheInfo> m_PedModelCacheInfoMap;
    std::map<ushort, SModelCacheInfo> m_VehicleModelCacheInfoMap;
    std::set<ushort>                  m_PermoLoadedModels;
};

///////////////////////////////////////////////////////////////
//
// Global new
//
//
///////////////////////////////////////////////////////////////
CModelCacheManager* NewModelCacheManager()
{
    return new CModelCacheManagerImpl();
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::CModelCacheManagerImpl
//
///////////////////////////////////////////////////////////////
CModelCacheManagerImpl::CModelCacheManagerImpl()
{
    m_pGame = CCore::GetSingleton().GetGame();
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::~CModelCacheManagerImpl
//
// Clean up when quitting
//
///////////////////////////////////////////////////////////////
CModelCacheManagerImpl::~CModelCacheManagerImpl()
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::OnClientClose
//
// Clean up when client.dll unloads
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::OnClientClose()
{
    // Remove all extra refs applied here
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::PreLoad
//
// Cache all weapons and upgrades
//
// Peds KB:                64,832 KB         7-312     306  296 valid, 10 not so valid               219   KB/model             4.45/MB
// Weapons KB:             470               321-372   52   39 valid, 3 invalid(329,332,340)         470   KB all weapons
// Upgrades KB:            2,716             1000-1193 194  all valid                              2,716   KB all upgrades
// Vehicles(400-499) KB:   14,622                                                                    140   KB/model             7/MB
// Vehicles(500-599) KB:   14,888
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::PreLoad()
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::GetStats
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::GetStats(SModelCacheStats& outStats)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::DoPulse
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::DoPulse()
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::AddModelToPersistentCache
//
// Keep this model around 4 evar now
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::AddModelToPersistentCache(ushort usModelId)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::UpdatePedModelCaching
//
//
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::UpdatePedModelCaching(const std::map<ushort, float>& newNeedCacheList)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::UpdateVehicleModelCaching
//
//
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::UpdateVehicleModelCaching(const std::map<ushort, float>& newNeedCacheList)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::RemoveCacheRefs
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::RemoveCacheRefs(std::map<ushort, SModelCacheInfo>& currentCacheInfoMap)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::UpdateModelCaching
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::UpdateModelCaching(const std::map<ushort, float>& newNeedCacheList, std::map<ushort, SModelCacheInfo>& currentCacheInfoMap,
                                                uint uiMaxCachedAllowed)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::GetModelRefCount
//
///////////////////////////////////////////////////////////////
int CModelCacheManagerImpl::GetModelRefCount(ushort usModelId)
{
    return 0;
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::AddModelRefCount
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::AddModelRefCount(ushort usModelId)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::SubModelRefCount
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::SubModelRefCount(ushort usModelId)
{
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::UnloadModel
//
// Remove model and associated txd from memory
//
///////////////////////////////////////////////////////////////
bool CModelCacheManagerImpl::UnloadModel(ushort usModelId)
{
    return true;
}

///////////////////////////////////////////////////////////////
//
// CModelCacheManagerImpl::OnRestreamModel
//
// Uncache here, now.
//
///////////////////////////////////////////////////////////////
void CModelCacheManagerImpl::OnRestreamModel(ushort usModelId)
{
}
