/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        SharedUtil.Math.h
 *  PURPOSE:
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/
#pragma once

#include <cmath>
#include "SharedUtil.Misc.h"

namespace SharedUtil
{
    // Bitwise constant operations
    template <unsigned int N>
    struct NumberOfSignificantBits
    {
        enum
        {
            COUNT = 1 + NumberOfSignificantBits<(N >> 1)>::COUNT
        };
    };
    template <>
    struct NumberOfSignificantBits<0>
    {
        enum
        {
            COUNT = 0
        };
    };

    template <class T>
    T Square(const T& value)
    {
        return value * value;
    }

    // Remove possible crap after casting a float to a double
    inline double RoundFromFloatSource(double dValue)
    {
        double dFactor = pow(10.0, 8 - ceil(log10(fabs(dValue))));
        return floor(dValue * dFactor + 0.5) / dFactor;
    }

    // Determine if value would be better as an int or float.
    // piNumber is set if result is true
    inline bool ShouldUseInt(double dValue, int* piNumber)
    {
        if (dValue > -0x1000000 && dValue < 0x1000000)
        {
            // float more accurate with this range, but check if we can use int as it is better for compressing
            *piNumber = static_cast<int>(dValue);
            return (dValue == *piNumber);
        }
        else if (dValue >= -0x7FFFFFFF && dValue <= 0x7FFFFFFF)
        {
            // int more accurate with this range
            *piNumber = Round(dValue);
            return true;
        }
        else
        {
            // Value too large for int
            return false;
        }
    }

    /*
     * Compress a given number into a possibly smaller datatype
     * might be lossy, as it might be rounded to an int.
     * Calls `visitor` with the "value" type
     */
    template<typename Visitor>
    inline void CompressArithmetic(double value, Visitor visitor)
    {
        if (value > -0x1000000 && value < 0x1000000) /* float / int */
        {
            /* float more accurate in this range than int
             * but check if we can use int, as it's better for compressing
             */
            if (int iValue{ static_cast<int>(value) }; iValue == value)
                visitor(iValue);
            else
                visitor(static_cast<float>(value));
        }
        else if (value >= -0x7FFFFFFF && value <= 0x7FFFFFFF) /* int */
        {
            visistor(static_cast<int>(value));
        }
        else /* double */
        {
            visitor(std::floor(dValue + 0.5)); /* rounded for consistency with previous range */
        }
    }

    inline float DegreesToRadians(float fValue) { return fValue * 0.017453292f; }
}            // namespace SharedUtil
