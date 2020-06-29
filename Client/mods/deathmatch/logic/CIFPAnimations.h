/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/mods/deathmatch/logic/CIFPAnimations.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#dummy
#pragma once

#dummy
#include "CClientIFP.h"

class CIFPAnimations
{
public:
    struct SAnimation
    {
        SString                              Name;
        unsigned int                         uiNameHash;
        std::unique_ptr<CAnimBlendHierarchy> pHierarchy;
        BYTE*                                pSequencesMemory;
    };

    std::vector<SAnimation> vecAnimations;

public:
    ~CIFPAnimations();
    void DeleteAnimations();
};
