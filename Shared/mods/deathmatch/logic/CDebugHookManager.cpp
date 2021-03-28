/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        CDebugHookManager.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#ifdef MTA_CLIENT
    #define DECLARE_PROFILER_SECTION_CDebugHookManager
    #include "profiler/SharedUtil.Profiler.h"
    #define g_pGame g_pClientGame
#else
    #define DECLARE_PROFILER_SECTION(tag)
#endif

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::CDebugHookManager
//
//
//
///////////////////////////////////////////////////////////////
CDebugHookManager::CDebugHookManager()
{
    m_MaskArgumentsMap = {
#ifndef MTA_CLIENT
                        {"logIn", {{EArgType::Password, 2}}},                           // player, account, 2=PASSWORD
                        {"addAccount", {{EArgType::Password, 1}}},                      // name, 1=PASSWORD
                        {"getAccount", {{EArgType::Password, 1}}},                      // name, 1=PASSWORD
                        {"setAccountPassword", {{EArgType::Password, 1}}},              // account, 1=PASSWORD
                        {"dbConnect", {{EArgType::MaxArgs, 0}}},
                        {"dbExec", {{EArgType::MaxArgs, 0}}},
                        {"dbFree", {{EArgType::MaxArgs, 0}}},
                        {"dbPoll", {{EArgType::MaxArgs, 0}}},
                        {"dbPrepareString", {{EArgType::MaxArgs, 0}}},
                        {"dbQuery", {{EArgType::MaxArgs, 0}}},
                        {"executeSQLQuery", {{EArgType::MaxArgs, 0}}},
                        {"callRemote", {{EArgType::MaxArgs, 1}, {EArgType::Url, 0}}},   // 0=URL, ...
#endif
                        {"fetchRemote", {{EArgType::MaxArgs, 1}, {EArgType::Url, 0}}},  // 0=URL, ...
                        {"passwordHash", {{EArgType::Password, 0}}},                    // 0=PASSWORD, ...
                        {"passwordVerify", {{EArgType::Password, 0}}},                  // 0=PASSWORD, ...
                        {"encodeString", {{EArgType::MaxArgs, 2}}},                     // algorithm, input, 2=SECRETKEY, ...
                        {"decodeString", {{EArgType::MaxArgs, 2}}},                     // algorithm, input, 2=SECRETKEY, ...
                        {"teaEncode", {{EArgType::Password, 1}}},                       // input, 1=SECRETKEY
                        {"teaDecode", {{EArgType::Password, 1}}},                       // input, 1=SECRETKEY
                    };
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::~CDebugHookManager
//
//
//
///////////////////////////////////////////////////////////////
CDebugHookManager::~CDebugHookManager()
{
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::GetHookInfoListForType
//
//
//
///////////////////////////////////////////////////////////////
std::vector<SDebugHookCallInfo>& CDebugHookManager::GetHookInfoListForType(EDebugHookType hookType)
{
    switch (hookType)
    {
        case EDebugHookType::PRE_EVENT:
            return m_PreEventHookList;
        case EDebugHookType::POST_EVENT:
            return m_PostEventHookList;
        case EDebugHookType::PRE_FUNCTION:
            return m_PreFunctionHookList;
        case EDebugHookType::POST_FUNCTION:
            return m_PostFunctionHookList;
        case EDebugHookType::PRE_EVENT_FUNCTION:
            return m_PreEventFunctionHookList;
        case EDebugHookType::POST_EVENT_FUNCTION:
            return m_PostEventFunctionHookList;
        case EDebugHookType::MAX_DEBUG_HOOK_TYPE:
        default:
            dassert(hookType == EDebugHook::POST_FUNCTION);
            return m_PostFunctionHookList;
    }
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::AddDebugHook
//
// Returns true if hook was added
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::AddDebugHook(EDebugHookType hookType, const CLuaFunctionRef& functionRef, const std::vector<SString>& allowedNameList)
{
    std::vector<SDebugHookCallInfo>& hookInfoList = GetHookInfoListForType(hookType);
    for (std::vector<SDebugHookCallInfo>::iterator iter = hookInfoList.begin(); iter != hookInfoList.end(); ++iter)
    {
        if ((*iter).functionRef == functionRef)
            return false;
    }

    SDebugHookCallInfo info;
    info.functionRef = functionRef;
    info.pLuaMain = g_pGame->GetLuaManager()->GetVirtualMachine(functionRef.GetLuaVM());
    if (!info.pLuaMain)
        return false;

    for (uint i = 0; i < allowedNameList.size(); i++)
        MapInsert(info.allowedNameMap, allowedNameList[i]);

    hookInfoList.push_back(info);
    return true;
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::RemoveDebugHook
//
// Returns true if hook was removed
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::RemoveDebugHook(EDebugHookType hookType, const CLuaFunctionRef& functionRef)
{
    CLuaMain* pLuaMain = g_pGame->GetLuaManager()->GetVirtualMachine(functionRef.GetLuaVM());

    std::vector<SDebugHookCallInfo>& hookInfoList = GetHookInfoListForType(hookType);
    for (std::vector<SDebugHookCallInfo>::iterator iter = hookInfoList.begin(); iter != hookInfoList.end(); ++iter)
    {
        if ((*iter).pLuaMain == pLuaMain && (*iter).functionRef == functionRef)
        {
            hookInfoList.erase(iter);
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnLuaMainDestroy
//
// When a Lua VM is stopped
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::OnLuaMainDestroy(CLuaMain* pLuaMain)
{
    for (uint hookType = EDebugHook::PRE_EVENT; hookType < EDebugHook::MAX_DEBUG_HOOK_TYPE; hookType++)
    {
        std::vector<SDebugHookCallInfo>& hookInfoList = GetHookInfoListForType((EDebugHookType)hookType);
        for (uint i = 0; i < hookInfoList.size();)
        {
            if (hookInfoList[i].pLuaMain == pLuaMain)
                ListRemoveIndex(hookInfoList, i);
            else
                i++;
        }
    }
}

///////////////////////////////////////////////////////////////
//
// GetDebugInfo
//
// Get current Lua source file and line number
//
///////////////////////////////////////////////////////////////
void GetDebugInfo(lua_State* luaVM, lua_Debug& debugInfo, const char*& szFilename, int& iLineNumber)
{
    if (luaVM && lua_getstack(luaVM, 1, &debugInfo))
    {
        lua_getinfo(luaVM, "nlS", &debugInfo);

        // Make sure this function isn't defined in a string
        if (debugInfo.source[0] == '@')
        {
            szFilename = debugInfo.source;
            iLineNumber = debugInfo.currentline != -1 ? debugInfo.currentline : debugInfo.linedefined;
        }
        else
        {
            szFilename = debugInfo.short_src;
        }

        // Remove path
        if (const char* szNext = strrchr(szFilename, '\\'))
            szFilename = szNext + 1;
        if (const char* szNext = strrchr(szFilename, '/'))
            szFilename = szNext + 1;
    }
}

///////////////////////////////////////////////////////////////
//
// GetMapEventDebugInfo
//
// Get current Lua source file and line number
//
///////////////////////////////////////////////////////////////
void GetMapEventDebugInfo(CMapEvent* pMapEvent, const char*& szFilename, int& iLineNumber)
{
    CLuaMain* pLuaMain = pMapEvent->GetVM();

    if (!pLuaMain)
        return;

    lua_State* luaVM = pLuaMain->GetVirtualMachine();

    if (!luaVM)
        return;

    const CLuaFunctionRef& iLuaFunction = pMapEvent->GetLuaFunction();
    lua_Debug              debugInfo;
    lua_getref(luaVM, iLuaFunction.ToInt());

    if (lua_getinfo(luaVM, ">lS", &debugInfo))
    {
        // Make sure this function isn't defined in a string
        if (debugInfo.source[0] == '@')
        {
            szFilename = debugInfo.source;
            iLineNumber = debugInfo.currentline != -1 ? debugInfo.currentline : debugInfo.linedefined;
        }
        else
        {
            szFilename = debugInfo.short_src;
        }

        // Remove path
        if (const char* szNext = strrchr(szFilename, '\\'))
            szFilename = szNext + 1;

        if (const char* szNext = strrchr(szFilename, '/'))
            szFilename = szNext + 1;
    }
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPreFunction
//
// Called before an MTA function is called
// Returns false if function call should be skipped
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::OnPreFunction(lua_CFunction f, lua_State* luaVM, bool bAllowed)
{
    DECLARE_PROFILER_SECTION(OnPreFunction)

    if (m_PreFunctionHookList.empty())
        return true;

    CLuaCFunction* pFunction = CLuaCFunctions::GetFunction(f);
    if (!pFunction)
        return true;

    const SString& strName = pFunction->GetName();
    bool           bNameMustBeExplicitlyAllowed = MustNameBeExplicitlyAllowed(strName);

    // Check if named function is pre hooked
    if (!IsNameAllowed(strName, m_PreFunctionHookList, bNameMustBeExplicitlyAllowed))
        return true;

    CValues NewArguments;
    GetFunctionCallHookArguments(NewArguments, strName, luaVM, bAllowed);

    return CallHook(strName, m_PreFunctionHookList, NewArguments, bNameMustBeExplicitlyAllowed);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPostFunction
//
// Called after an MTA function is called
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::OnPostFunction(lua_CFunction f, lua_State* luaVM)
{
    DECLARE_PROFILER_SECTION(OnPostFunction)

    if (m_PostFunctionHookList.empty())
        return;

    CLuaCFunction* pFunction = CLuaCFunctions::GetFunction(f);
    if (!pFunction)
        return;

    const SString& strName = pFunction->GetName();
    bool           bNameMustBeExplicitlyAllowed = MustNameBeExplicitlyAllowed(strName);

    // Check if named function is post hooked
    if (!IsNameAllowed(strName, m_PostFunctionHookList, bNameMustBeExplicitlyAllowed))
        return;

    CValues NewArguments;
    GetFunctionCallHookArguments(NewArguments, strName, luaVM, true);

    CallHook(strName, m_PostFunctionHookList, NewArguments, bNameMustBeExplicitlyAllowed);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::GetFunctionCallHookArguments
//
// Get call hook arguments for OnPre/PostFunction
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::GetFunctionCallHookArguments(CValues& NewArguments, const SString& strName, lua_State* luaVM, bool bAllowed)
{
    // Get file/line number
    const char* szFilename = "";
    int         iLineNumber = 0;
    lua_Debug   debugInfo;
    GetDebugInfo(luaVM, debugInfo, szFilename, iLineNumber);

    CLuaMain*  pSourceLuaMain = g_pGame->GetScriptDebugging()->GetTopLuaMain();
    CResource* pSourceResource = pSourceLuaMain ? pSourceLuaMain->GetResource() : NULL;

    if (pSourceResource)
        NewArguments.Push(pSourceResource);
    else
        NewArguments.PushNil();
    NewArguments.Push(strName);
    NewArguments.Push(bAllowed);
    NewArguments.Push(szFilename);
    NewArguments.Push(iLineNumber);

    CValues FunctionArguments;
    FunctionArguments.ReadAll(luaVM);
    MaybeMaskArgumentValues(strName, FunctionArguments);
    NewArguments.Write(FunctionArguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPreEvent
//
// Called before a Lua event is triggered
// Returns false if event should be skipped
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::OnPreEvent(const char* szName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller)
{
    if (m_PreEventHookList.empty())
        return true;

    // Check if named event is pre hooked
    if (!IsNameAllowed(szName, m_PreEventHookList))
        return true;

    CValues NewArguments;
    GetEventCallHookArguments(NewArguments, szName, Arguments, pSource, pCaller);

    return CallHook(szName, m_PreEventHookList, NewArguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPostEvent
//
// Called after a Lua event is triggered
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::OnPostEvent(const char* szName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller)
{
    if (m_PostEventHookList.empty())
        return;

    // Check if named event is post hooked
    if (!IsNameAllowed(szName, m_PostEventHookList))
        return;

    CValues NewArguments;
    GetEventCallHookArguments(NewArguments, szName, Arguments, pSource, pCaller);

    CallHook(szName, m_PostEventHookList, NewArguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::GetEventCallHookArguments
//
// Get call hook arguments for OnPre/PostEvent
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::GetEventCallHookArguments(CValues& NewArguments, const SString& strName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller)
{
    CLuaMain*  pSourceLuaMain = g_pGame->GetScriptDebugging()->GetTopLuaMain();
    CResource* pSourceResource = pSourceLuaMain ? pSourceLuaMain->GetResource() : NULL;

    // Get file/line number
    const char* szFilename = "";
    int         iLineNumber = 0;
    lua_Debug   debugInfo;
    lua_State*  luaVM = pSourceLuaMain ? pSourceLuaMain->GetVM() : NULL;
    if (luaVM)
        GetDebugInfo(luaVM, debugInfo, szFilename, iLineNumber);

    if (pSourceResource)
        NewArguments.Push(pSourceResource);
    else
        NewArguments.PushNil();
    NewArguments.Push(strName);
    NewArguments.Push(pSource);
    NewArguments.Push(pCaller);
    NewArguments.Push(szFilename);
    NewArguments.Push(iLineNumber);
    NewArguments.Write(Arguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPreEventFunction
//
// Called before a Lua event function is called
// Returns false if function call should be skipped
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::OnPreEventFunction(const char* szName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller, CMapEvent* pMapEvent)
{
    if (m_PreEventFunctionHookList.empty())
        return true;

    // Check if named event function is pre hooked
    if (!IsNameAllowed(szName, m_PreEventFunctionHookList))
        return true;

    CValues NewArguments;
    GetEventFunctionCallHookArguments(NewArguments, szName, Arguments, pSource, pCaller, pMapEvent);

    return CallHook(szName, m_PreEventFunctionHookList, NewArguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::OnPostEventFunction
//
// Called after a Lua event function is called
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::OnPostEventFunction(const char* szName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller, CMapEvent* pMapEvent)
{
    if (m_PostEventFunctionHookList.empty())
        return;

    // Check if named event function is post hooked
    if (!IsNameAllowed(szName, m_PostEventFunctionHookList))
        return;

    CValues NewArguments;
    GetEventFunctionCallHookArguments(NewArguments, szName, Arguments, pSource, pCaller, pMapEvent);

    CallHook(szName, m_PostEventFunctionHookList, NewArguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::GetEventFunctionCallHookArguments
//
// Get call hook arguments for OnPre/PostEventFunction
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::GetEventFunctionCallHookArguments(CValues& NewArguments, const SString& strName, const CValues& Arguments, CElement* pSource, CPlayer* pCaller, CMapEvent* pMapEvent)
{
    CLuaMain*  pEventLuaMain = g_pGame->GetScriptDebugging()->GetTopLuaMain();
    CResource* pEventResource = pEventLuaMain ? pEventLuaMain->GetResource() : NULL;

    // Get file/line number for event
    const char* szEventFilename = "";
    int         iEventLineNumber = 0;
    lua_Debug   eventDebugInfo;
    lua_State*  eventLuaVM = pEventLuaMain ? pEventLuaMain->GetVM() : NULL;
    if (eventLuaVM)
        GetDebugInfo(eventLuaVM, eventDebugInfo, szEventFilename, iEventLineNumber);

    // Get file/line number for function
    const char* szFunctionFilename = "";
    int         iFunctionLineNumber = 0;
    GetMapEventDebugInfo(pMapEvent, szFunctionFilename, iFunctionLineNumber);

    CLuaMain*  pFunctionLuaMain = pMapEvent->GetVM();
    CResource* pFunctionResource = pFunctionLuaMain ? pFunctionLuaMain->GetResource() : NULL;

    // resource eventResource, string eventName, element eventSource, element eventClient, string eventFilename, int eventLineNumber,
    if (pEventResource)
        NewArguments.Push(pEventResource);
    else
        NewArguments.PushNil();

    NewArguments.Push(strName);
    NewArguments.Push(pSource);
    NewArguments.Push(pCaller);
    NewArguments.Push(szEventFilename);
    NewArguments.Push(iEventLineNumber);

    // resource functionResource, string functionFilename, int functionLineNumber, ...args
    if (pFunctionResource)
        NewArguments.Push(pFunctionResource);
    else
        NewArguments.PushNil();

    NewArguments.Push(szFunctionFilename);
    NewArguments.Push(iFunctionLineNumber);
    NewArguments.Write(Arguments);
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::IsNameAllowed
//
// Returns true if there is a debughook which handles the name
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::IsNameAllowed(const char* szName, const std::vector<SDebugHookCallInfo>& eventHookList, bool bNameMustBeExplicitlyAllowed)
{
    for (uint i = 0; i < eventHookList.size(); i++)
    {
        const SDebugHookCallInfo& info = eventHookList[i];

        if (info.allowedNameMap.empty() && !bNameMustBeExplicitlyAllowed)
            return true;            // All names allowed

        if (MapContains(info.allowedNameMap, szName))
            return true;            // Name allowed
    }
    return false;
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::MustNameBeExplicitlyAllowed
//
// Don't trace add/removeDebugHook unless requested
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::MustNameBeExplicitlyAllowed(const SString& strName)
{
    return strName == "addDebugHook" || strName == "removeDebugHook";
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::MaybeMaskArgumentValues
//
// Mask security sensitive argument values
//
///////////////////////////////////////////////////////////////
void CDebugHookManager::MaybeMaskArgumentValues(const SString& strFunctionName, CValues& FunctionArguments)
{
    auto* pMaskArgumentList = MapFind(m_MaskArgumentsMap, strFunctionName);
    if (pMaskArgumentList)
    {
        for (const auto& maskArgument : *pMaskArgumentList)
        {
            if (maskArgument.argType == EArgType::Password)
            {
                CValue* pArgument = FunctionArguments[maskArgument.index];
                if (pArgument && !pArgument->GetString().empty())
                    pArgument->ReadString("***");
            }
            else if (maskArgument.argType == EArgType::Url)
            {
                CValue* pArgument = FunctionArguments[maskArgument.index];
                if (pArgument)
                {
                    // Remove query portion of URL
                    SString strUrlCleaned = SString(pArgument->GetString()).ReplaceI("%3F","?").Replace("#","?").SplitLeft("?");
                    pArgument->ReadString(strUrlCleaned);
                }
            }
            else if (maskArgument.argType == EArgType::MaxArgs)
            {
                while(FunctionArguments.Count() > maskArgument.index)
                    FunctionArguments.Pop();
            }
        }
    }
}

///////////////////////////////////////////////////////////////
//
// CDebugHookManager::CallHook
//
// Return false if function/event should be skipped
//
///////////////////////////////////////////////////////////////
bool CDebugHookManager::CallHook(const char* szName, const std::vector<SDebugHookCallInfo>& eventHookList, const CValues& Arguments,
                                 bool bNameMustBeExplicitlyAllowed)
{
    static bool bRecurse = false;
    if (bRecurse)
        return true;
    bRecurse = true;
    bool bSkip = false;

    for (uint i = 0; i < eventHookList.size(); i++)
    {
        const SDebugHookCallInfo& info = eventHookList[i];

        if (!info.allowedNameMap.empty() || bNameMustBeExplicitlyAllowed)
        {
            if (!MapContains(info.allowedNameMap, szName))
                continue;
        }

        lua_State* pState = info.pLuaMain->GetVirtualMachine();

        if (!pState)
            continue;

        // Save script MTA globals in case hook messes with them
        lua_getglobal(pState, "source");
        CValue OldSource(pState, -1);
        lua_pop(pState, 1);

        lua_getglobal(pState, "this");
        CValue OldThis(pState, -1);
        lua_pop(pState, 1);

        lua_getglobal(pState, "sourceResource");
        CValue OldResource(pState, -1);
        lua_pop(pState, 1);

        lua_getglobal(pState, "sourceResourceRoot");
        CValue OldResourceRoot(pState, -1);
        lua_pop(pState, 1);

        lua_getglobal(pState, "eventName");
        CValue OldEventName(pState, -1);
        lua_pop(pState, 1);

        lua_getglobal(pState, "client");
        CValue OldClient(pState, -1);
        lua_pop(pState, 1);

        CValues returnValues;
        Arguments.Call(info.pLuaMain, info.functionRef, &returnValues);
        // Note: info could be invalid now

        // Check for skip option
        if (returnValues.Count())
        {
            CValue* returnedValue = *returnValues.IterBegin();
            if (returnedValue->GetType() == LUA_TSTRING)
            {
                if (returnedValue->GetString() == "skip")
                    bSkip = true;
            }
        }

        // Reset the globals on that VM
        OldSource.Push(pState);
        lua_setglobal(pState, "source");

        OldThis.Push(pState);
        lua_setglobal(pState, "this");

        OldResource.Push(pState);
        lua_setglobal(pState, "sourceResource");

        OldResourceRoot.Push(pState);
        lua_setglobal(pState, "sourceResourceRoot");

        OldEventName.Push(pState);
        lua_setglobal(pState, "eventName");

        OldClient.Push(pState);
        lua_setglobal(pState, "client");
    }

    bRecurse = false;
    return !bSkip;
}
