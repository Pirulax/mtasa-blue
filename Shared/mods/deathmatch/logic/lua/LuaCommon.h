/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/lua/LuaCommon.h
 *  PURPOSE:     Lua common functions
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

CLuaFunctionRef luaM_toref(lua_State* luaVM, int iArgument);

#define TO_ELEMENTID(x) ((ElementID) reinterpret_cast < unsigned long > (x) )

#ifdef MTA_CLIENT
// Predeclarations of our classes
class CClientColModel;
class CClientColShape;
class CScriptFile;
class CClientDFF;
class CClientEntity;
class CClientGUIElement;
class CClientMarker;
class CClientObject;
class CClientPed;
class CClientPickup;
class CClientPlayer;
class CClientRadarMarker;
class CClientTeam;
class CClientTXD;
class CClientVehicle;
class CClientWater;
class CClientWeapon;
class CClientRadarArea;
class CClientPointLights;
class CLuaTimer;
class CResource;
class CXMLNode;

// Lua pop macros for our datatypes
class CClientEntity* lua_toelement(lua_State* luaVM, int iArgument);
#else
// Lua pop macros for our datatypes
class CElement* lua_toelement(lua_State* luaVM, int iArgument);
#endif

template<class T>
void lua_pushobject(lua_State* luaVM, T* object);

void lua_pushuserdata(lua_State* luaVM, void* value);
void lua_pushobject(lua_State* luaVM, const char* szClass, void* pObject, bool bSkipCache = false);

#ifdef MTA_CLIENT
void lua_pushobject(lua_State* luaVM, CClientEntity* player);
#else
void lua_pushobject(lua_State* luaVM, CElement* player);
void lua_pushobject(lua_State* luaVM, CPlayer* player);
#endif
void lua_pushobject(lua_State* luaVM, const CVector2D& vector);
void lua_pushobject(lua_State* luaVM, const CVector& vector);
void lua_pushobject(lua_State* luaVM, const CVector4D& vector);
void lua_pushobject(lua_State* luaVM, const CMatrix& matrix);

template<class T>
void lua_pushobject(lua_State* luaVM, T* object)
{
    static_assert(!std::is_pointer_v<T> && !std::is_reference_v<T>, "T must be an object, not a pointer to a pointer, or something..");

    const auto push = [luaVM, object](const auto& value) {
        lua_pushobject(luaVM, GetClassNameIfOOPEnabled(luaVM, object), (void*)reinterpret_cast<unsigned int*>(value))
    };

    using Decayed_t = std::decay_t<T>;
    if constexpr (std::is_same_v<CXMLNode, Decayed_t>)
        push(object->GetID());

    else if constexpr (std::is_same_v<CDbJobData, Decayed_t>)
        push(object->GetId());

    else if constexpr (std::is_base_of_v<CElement, Decayed_t>) // Handle types that derive from CElement
    {
        if (object->IsBeingDeleted())
            lua_pushboolean(luaVM, false);
        else if (const auto ID = object->GetID(); ID != INVALID_ELEMENT_ID)
            push(ID.Value());
        else
            lua_pushnil(luaVM); // Invalid element ID
    }
#ifndef MTA_CLIENT
    else if constexpr (std::is_base_of_v<CClient, Decayed_t>) // Handle types deriving from CClient (such as CPlayer)
        lua_pushobject<CElement>(luaVM, static_cast<CClient*>(object)->GetElement()); // Get the underlaying element, and call us
#endif
    else // Everything else should work with this. If not just add an std::is_same_v before this
        lua_pushobject(luaVM, szClass, (void*)reinterpret_cast<unsigned int*>(object->GetScriptID()));
}

// Internal use
void lua_initclasses(lua_State* luaVM);

void lua_newclass(lua_State* luaVM);
void lua_getclass(lua_State* luaVM, const char* szName);
void lua_registerclass(lua_State* luaVM, const char* szName, const char* szParent = NULL, bool bRegisterWithEnvironment = true);
void lua_registerstaticclass(lua_State* luaVM, const char* szName);
void lua_classfunction(lua_State* luaVM, const char* szFunction, const char* fn);
void lua_classvariable(lua_State* luaVM, const char* szVariable, const char* set, const char* get);
void lua_classmetamethod(lua_State* luaVM, const char* szName, lua_CFunction fn);

#ifdef MTA_CLIENT
void lua_classfunction(lua_State* luaVM, const char* szFunction, lua_CFunction fn);
void lua_classvariable(lua_State* luaVM, const char* szVariable, lua_CFunction set, lua_CFunction get);
#else
void lua_classfunction(lua_State* luaVM, const char* szFunction, const char* szACLName, lua_CFunction fn);
void lua_classvariable(lua_State* luaVM, const char* szVariable, const char* szACLNameSet, const char* szACLNameGet, lua_CFunction set, lua_CFunction get, bool bACLIgnore = true);
#endif

// Include the RPC functions enum
#include "net/rpc_enums.h"

enum
{
    AUDIO_FRONTEND,
    AUDIO_MISSION_PRELOAD,
    AUDIO_MISSION_PLAY
};

// Lua debug info for logging
enum
{
    DEBUG_INFO_NONE,
    DEBUG_INFO_FILE_AND_LINE,
    DEBUG_INFO_SHORT_SRC,
};

#define INVALID_LINE_NUMBER (-1)

struct SLuaDebugInfo
{
    SLuaDebugInfo() : iLine(INVALID_LINE_NUMBER), infoType(DEBUG_INFO_NONE) {}
    SLuaDebugInfo(const SString& strFile, int iLine, const SString& strShortSrc = "")
        : strFile(strFile), strShortSrc(strShortSrc), iLine(iLine), infoType(DEBUG_INFO_FILE_AND_LINE)
    {
    }
    SString strFile;
    SString strShortSrc;
    int     iLine;
    int     infoType;
};
