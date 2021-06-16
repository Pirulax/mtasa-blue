/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "../../../core/CModelCacheManager.h"

#define PED_STREAM_IN_DISTANCE              (250)
#define VEHICLE_STREAM_IN_DISTANCE          (250)
#define STREAMER_STREAM_OUT_EXTRA_DISTANCE  (50)

#define PED_MAX_STREAM_DISTANCE             ( PED_STREAM_IN_DISTANCE + STREAMER_STREAM_OUT_EXTRA_DISTANCE )
#define PED_MAX_STREAM_DISTANCE_SQ          ( PED_MAX_STREAM_DISTANCE * PED_MAX_STREAM_DISTANCE )

#define VEHICLE_MAX_STREAM_DISTANCE         ( VEHICLE_STREAM_IN_DISTANCE + STREAMER_STREAM_OUT_EXTRA_DISTANCE )
#define VEHICLE_MAX_STREAM_DISTANCE_SQ      ( VEHICLE_MAX_STREAM_DISTANCE * VEHICLE_MAX_STREAM_DISTANCE )

#define PED_MAX_VELOCITY                    (10)
#define VEHICLE_MAX_VELOCITY                (10)

///////////////////////////////////////////////////////////////
//
//
// CClientModelCacheManager
//
//
///////////////////////////////////////////////////////////////
class CClientModelCacheManagerImpl : public CClientModelCacheManager
{
public:
    ZERO_ON_NEW

    // CClientModelCacheManager interface
    virtual void DoPulse();
    virtual void OnRestreamModel(ushort usModelId);

    // CClientModelCacheManagerImpl methods
    CClientModelCacheManagerImpl();
    ~CClientModelCacheManagerImpl();

    void DoPulsePedModels();
    void DoPulseVehicleModels();
    void ProcessPlayerList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientPlayer*>& playerList, float fMaxStreamDistanceSq);
    void ProcessPedList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientPed*>& pedList, float fMaxStreamDistanceSq);
    void ProcessVehicleList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientVehicle*>& vehicleList, float fMaxStreamDistanceSq);
    void InsertIntoNeedCacheList(std::map<ushort, float>& outNeedCacheList, ushort usModelId, float fDistSq);
    void ClearStats();
    void AddProcessStat(const char* szTag, bool bCache, ePuresyncType syncType, ushort usModelId, const CVector& vecStartPos, const CVector& vecEndPos);

protected:
    int                 m_iFrameCounter;
    CTickCount          m_TickCountNow;
    CVector             m_vecCameraPos;
    CTickCount          m_LastTimeMs;
    float               m_fSmoothCameraSpeed;
    CClientPlayer*      m_pLocalPlayer;
    float               m_fGameFps;
    CModelCacheManager* m_pCoreModelCacheManager;
};

///////////////////////////////////////////////////////////////
//
// Global new
//
//
///////////////////////////////////////////////////////////////
CClientModelCacheManager* NewClientModelCacheManager()
{
    return new CClientModelCacheManagerImpl();
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::CClientModelCacheManagerImpl
//
///////////////////////////////////////////////////////////////
CClientModelCacheManagerImpl::CClientModelCacheManagerImpl()
{
    m_pCoreModelCacheManager = g_pCore->GetModelCacheManager();
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::~CClientModelCacheManagerImpl
//
///////////////////////////////////////////////////////////////
CClientModelCacheManagerImpl::~CClientModelCacheManagerImpl()
{
    m_pCoreModelCacheManager->OnClientClose();
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::DoPulse
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::DoPulse()
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::DoPulsePedModels
//
// Pulse caching system for ped models
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::DoPulsePedModels()
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::DoPulseVehicleModels
//
// Pulse caching system for vehicle models
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::DoPulseVehicleModels()
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::ProcessPlayerList
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::ProcessPlayerList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientPlayer*>& playerList,
                                                     float fMaxStreamDistanceSq)
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::ProcessPedList
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::ProcessPedList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientPed*>& pedList,
                                                  float fMaxStreamDistanceSq)
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::ProcessVehicleList
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::ProcessVehicleList(std::map<ushort, float>& outNeedCacheList, const std::vector<CClientVehicle*>& vehicleList,
                                                      float fMaxStreamDistanceSq)
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::InsertIntoNeedCacheList
//
// Update model id closest distance
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::InsertIntoNeedCacheList(std::map<ushort, float>& outNeedCacheList, ushort usModelId, float fDistSq)
{
}

//
//
//
//
// Stats
//
//
//
//

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::ClearStats
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::ClearStats()
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::AddProcessStat
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::AddProcessStat(const char* szTag, bool bCache, ePuresyncType syncType, ushort usModelId, const CVector& vecStartPos,
                                                  const CVector& vecEndPos)
{
}

///////////////////////////////////////////////////////////////
//
// CClientModelCacheManagerImpl::OnRestreamModel
//
// Uncache here, now.
//
///////////////////////////////////////////////////////////////
void CClientModelCacheManagerImpl::OnRestreamModel(ushort usModelId)
{
    m_pCoreModelCacheManager->OnRestreamModel(usModelId);
}
