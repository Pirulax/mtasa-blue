/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CCollisionSA.h
 *  PURPOSE:     Header file for `CCollision` - collision detection related functions
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include "game/CCollision.h"
#include "CCompressedVectorSA.h"
#include "CColModelSA.h"

class CCollisionSA : public CCollision {
public:
    CCollisionSA();

    bool TestLineSphere(const CColLineSA& line, const CColSphereSA& sphere) const override;

private:
    static bool TestSphereTriangle_AVX2(
        const CColSphereSA& sphere,
        const CCompressedVectorSA* verts,
        const CColTriangleSA& tri,
        const CColTrianglePlaneSA& plane
    );
    static bool TestSphereTriangle_Vanilla(
        const CColSphereSA& sphere,
        const CCompressedVectorSA* verts,
        const CColTriangleSA& tri,
        const CColTrianglePlaneSA& plane
    );
    void InstallHook_TestSphereTriangle();
};
