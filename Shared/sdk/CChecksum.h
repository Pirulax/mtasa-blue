/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        sdk/CChecksum.h
 *  PURPOSE:     Checksum class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#ifndef __CChecksum_H
#define __CChecksum_H

// Depends on CMD5Hasher and CRCGenerator

class CChecksum
{
public:
    CChecksum() noexcept = default;

    // Comparison operators
    bool operator==(const CChecksum& other) const { return ulCRC == other.ulCRC && memcmp(md5.data, other.md5.data, sizeof(md5.data)) == 0; }

    bool operator!=(const CChecksum& other) const { return !operator==(other); }

    operator bool() const { return bIsValid; }

    // static generators
    static CChecksum GenerateChecksumFromFile(const SString& strFilename)
    {
        CChecksum result;
        result.ulCRC = CRCGenerator::GetCRCFromFile(strFilename);
        CMD5Hasher().Calculate(strFilename, result.md5);
        result.bIsValid = true;

        return result;
    }

    static CChecksum GenerateChecksumFromBuffer(const char* cpBuffer, unsigned long ulLength)
    {
        CChecksum result;
        result.ulCRC = CRCGenerator::GetCRCFromBuffer(cpBuffer, ulLength);
        CMD5Hasher().Calculate(cpBuffer, ulLength, result.md5);
        result.bIsValid = true;

        return result;
    }

    unsigned long ulCRC = 0;
    MD5           md5 = { 0 };
    bool          bIsValid = false;
};

#endif
