/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CResourceFile.h
 *  PURPOSE:     Resource server-side file item class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

// This class controls a single resource file. This could be
// any item contained within the resource, mainly being a
// map or script.

// This is just a base class.

class CResourceFile;

#pragma once

#include "CChecksum.h"
#include "ehs/ehs.h"

class CResourceFile
{
public:
    enum eResourceType
    {
        RESOURCE_FILE_TYPE_MAP,
        RESOURCE_FILE_TYPE_SCRIPT,
        RESOURCE_FILE_TYPE_CLIENT_SCRIPT,
        RESOURCE_FILE_TYPE_CONFIG,
        RESOURCE_FILE_TYPE_CLIENT_CONFIG,
        RESOURCE_FILE_TYPE_HTML,
        RESOURCE_FILE_TYPE_CLIENT_FILE,
        RESOURCE_FILE_TYPE_NONE,
    };            // TODO: sort all client-side enums and use >= (instead of each individual type) on comparisons that use this enum?

protected:
    class CResource*    m_resource;
    std::string         m_strResourceFileName;            // full path
    std::string         m_strShortName;                   // just the filename
    std::string         m_strWindowsName;                 // the name with backwards slashes
    eResourceType       m_type;
    class CLuaMain*     m_pVM;
    CChecksum           m_checksum;            // Checksum last time this was loaded, generated by GenerateChecksum()
    uint                m_uiFileSize;
    map<string, string> m_attributeMap;            // Map of attributes from the meta.xml file

public:
    CResourceFile(class CResource* resource, const char* szShortName, const char* szResourceFileName, CXMLAttributes* xmlAttributes);
    virtual ~CResourceFile();

    virtual ResponseCode Request(HttpRequest* ipoHttpRequest, HttpResponse* ipoHttpResponse);

    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool IsNoClientCache() const { return false; }

    eResourceType GetType() { return m_type; }
    const char*   GetName() { return m_strShortName.c_str(); }
    const char*   GetFullName() { return m_strResourceFileName.c_str(); }
    const char*   GetWindowsName() { return m_strWindowsName.c_str(); }

    CChecksum GetLastChecksum() { return m_checksum; }
    void      SetLastChecksum(CChecksum checksum) { m_checksum = checksum; }
    void      SetLastFileSize(uint uiFileSize) { m_uiFileSize = uiFileSize; }

    double  GetApproxSize() { return m_uiFileSize; }            // Only used by download counters
    string  GetMetaFileAttribute(const string& key) { return m_attributeMap[key]; }
    SString GetCachedPathFilename(bool bForceClientCachePath = false);
};
