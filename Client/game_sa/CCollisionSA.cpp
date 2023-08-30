/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        game_sa/CCollisionSA.cpp
 *  PURPOSE:     Implementation of `CCollision` - collision detection related functions
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#include "CCollisionSA.h"

CCollisionSA::CCollisionSA() {
    InstallHook_TestSphereTriangle();
    
}

bool CCollisionSA::TestLineSphere(const CColLineSA& line, const CColSphereSA& sphere) const
{
    return reinterpret_cast<bool(__cdecl*)(const CColLineSA&, const CColSphereSA&)>(0x417470)(line, sphere);
}

bool CCollisionSA::TestSphereTriangle_Vanilla(const CColSphereSA& sphere, const CCompressedVectorSA* verts, const CColTriangleSA& tri, const CColTrianglePlaneSA& plane)
{
    const auto UncompressVector = [](const CCompressedVectorSA& cv, float divisor) -> CVector {
        return {
            static_cast<float>(cv.x) / divisor,
            static_cast<float>(cv.y) / divisor,
            static_cast<float>(cv.z) / divisor
        };
    };

    const auto P = sphere.m_center;
    const auto r = sphere.m_radius;
    
    const auto A  = UncompressVector(verts[tri.m_indices[0]], 128.f) - P;
    const auto B  = UncompressVector(verts[tri.m_indices[1]], 128.f) - P;
    const auto C  = UncompressVector(verts[tri.m_indices[2]], 128.f) - P;
    const auto rr = r * r;
    const auto N  = UncompressVector(plane.m_normal, 4096.f);
    const int  s1 = std::abs(A.Dot(N)) > r;
    const auto aa = A.Dot(A);
    const auto ab = A.Dot(B);
    const auto ac = A.Dot(C);
    const auto bb = B.Dot(B);
    const auto bc = B.Dot(C);
    const auto cc = C.Dot(C);
    const int  s2 = (aa > rr) & (ab > aa) & (ac > aa);
    const int  s3 = (bb > rr) & (ab > bb) & (bc > bb);
    const int  s4 = (cc > rr) & (ac > cc) & (bc > cc);
    const auto AB = B - A;
    const auto BC = C - B;
    const auto CA = A - C;
    const auto d1 = ab - aa;
    const auto d2 = bc - bb;
    const auto d3 = ac - cc;
    const auto e1 = AB.Dot(AB);
    const auto e2 = BC.Dot(BC);
    const auto e3 = CA.Dot(CA);
    const auto Q1 = A * e1 - AB * d1;
    const auto Q2 = B * e2 - BC * d2;
    const auto Q3 = C * e3 - CA * d3;
    const auto QC = C * e1 - Q1;
    const auto QA = A * e2 - Q2;
    const auto QB = B * e3 - Q3;
    const int  s5 = (Q1.Dot(Q1) > rr * e1 * e1) & (Q1.Dot(QC) > 0);
    const int  s6 = (Q2.Dot(Q2) > rr * e2 * e2) & (Q2.Dot(QA) > 0);
    const int  s7 = (Q3.Dot(Q3) > rr * e3 * e3) & (Q3.Dot(QB) > 0);

    return (s1 | s2 | s3 | s4 | s5 | s6 | s7) == 0;
}

bool CCollisionSA::TestSphereTriangle_AVX2(const CColSphereSA& sphere, const CCompressedVectorSA* verts, const CColTriangleSA& tri, const CColTrianglePlaneSA& plane)
{
    // Code based on: https://realtimecollisiondetection.net/blog/?p=103

    // Convert the compressed position and normal into a CVector
    //CVector uncompressed[4];
    //{
    //    const auto divisor = _mm256_set1_ps(1.0f / 128.0f);
    //
    //    alignas(32) short alignedData[3 * 4];  // 3 shorts per vector * 4 vectors
    //    memcpy(&alignedData[3 * 0], &verts[tri.m_indices[0]], sizeof(CCompressedVectorSA));
    //    memcpy(&alignedData[3 * 1], &verts[tri.m_indices[1]], sizeof(CCompressedVectorSA));
    //    memcpy(&alignedData[3 * 2], &verts[tri.m_indices[2]], sizeof(CCompressedVectorSA));
    //    memcpy(&alignedData[3 * 3], &plane.m_normal, sizeof(CCompressedVectorSA));
    //
    //    // Now we can load the values into AVX registers.
    //    // Each register contains 6 shorts [12 bytes]
    //    __m128i data[2]{
    //        _mm_load_si128(reinterpret_cast<__m128i*>(alignedData + 0)),
    //        _mm_load_si128(reinterpret_cast<__m128i*>(alignedData + 6))
    //    };
    //
    //    // Convert the short integers to packed single-precision floating-point values
    //    __m256 result[2]{
    //        _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(data[0])),
    //        _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(data[1]))
    //    };
    //    
    //    // Divide the converted values by the divisor using AVX2 intrinsics
    //    resultX = _mm256_div_ps(resultX, divisor);
    //    resultY = _mm256_div_ps(resultY, divisor);
    //
    //}

    // 3 vertices and the normal
    //__m128 A, B, C, N;
    //{
    //    const auto m = _mm256_set1_ps(1.0f / 128.0f);
    //    const auto Unpack = [](const CCompressedVectorSA& v) {
    //        return _mm_loadu_epi16
    //        };
    //}
   return false;
}

void CCollisionSA::InstallHook_TestSphereTriangle()
{
    HookInstall(0x4165B0, TestSphereTriangle_Vanilla);
}
