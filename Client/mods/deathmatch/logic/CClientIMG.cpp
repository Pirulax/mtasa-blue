/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CClientIMG.cpp
 *  PURPOSE:     IMG manager class
 *
 *****************************************************************************/

#include <StdInc.h>

CClientIMG::CClientIMG(class CClientManager* pManager, ElementID ID) : ClassInit(this), CClientEntity(ID)
{
    // Init
    m_pManager = pManager;
    SetTypeName("img");
}

CClientIMG::~CClientIMG()
{

}

bool CClientIMG::Load(SString sFilePath)
{
    if (sFilePath.empty())
        return false;

    m_strFilename = std::move(sFilePath);

/*    g_pClientGame->GetResourceManager()->ValidateResourceFile(m_strFilename, nullptr, 0);
    if (!g_pCore->GetNetwork()->CheckFile("img", m_strFilename))
        return false;
*/

    Stream();
    return true;
}

bool CClientIMG::Stream()
{
    m_ucStreamID = g_pGame->GetStreaming()->AddStreamHandler(*m_strFilename);
    return true;
}

bool CClientIMG::LinkModel(unsigned short usModelID, unsigned short usOffestInBlocks, unsigned short usSizeInBlocks)
{
    if (!m_ucStreamID)
        return false;

    g_pGame->GetStreaming()->SetModelStreamInfo(usModelID, m_ucStreamID, usOffestInBlocks, usSizeInBlocks);
    return true;
}
