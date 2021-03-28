/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#define MIN_CLIENT_REQ_CALLREMOTE_QUEUE_NAME         "1.5.3-9.11270"
#define MIN_CLIENT_REQ_FETCHREMOTE_CONNECT_TIMEOUT   "1.3.5"
#define MIN_CLIENT_REQ_CALLREMOTE_OPTIONS_TABLE      "1.5.4-9.11342"
#define MIN_CLIENT_REQ_CALLREMOTE_OPTIONS_FORMFIELDS "1.5.4-9.11413"

void CLuaNetworkDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"fetchRemote", FetchRemote},
        {"getRemoteRequests", GetRemoteRequests},
        {"getRemoteRequestInfo", GetRemoteRequestInfo},
        {"abortRemoteRequest", AbortRemoteRequest},
    };

    // Add functions
    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

// Call a function on a remote server
int CLuaNetworkDefs::FetchRemote(lua_State* luaVM)
{
    //  bool fetchRemote ( string URL [, string queueName ][, int connectionAttempts = 10, int connectTimeout = 10000 ], callback callbackFunction, [ string
    //  postData, bool bPostBinary, arguments... ] ) bool fetchRemote ( string URL [, table options ], callback callbackFunction[, table callbackArguments ] )
    CScriptArgReader    argStream(luaVM);
    SString             strURL;
    SHttpRequestOptions httpRequestOptions;
    SString             strQueueName;
    CLuaFunctionRef     iLuaFunction;
    CValues       callbackArguments;

    argStream.ReadString(strURL);
    if (!argStream.NextIsTable())
    {
        if (argStream.NextIsString())
            MinClientReqCheck(argStream, MIN_CLIENT_REQ_CALLREMOTE_QUEUE_NAME, "'queue name' is being used");
        argStream.ReadIfNextIsString(strQueueName, CALL_REMOTE_DEFAULT_QUEUE_NAME);
        argStream.ReadIfNextIsNumber(httpRequestOptions.uiConnectionAttempts, 10);
        if (argStream.NextIsNumber())
            MinClientReqCheck(argStream, MIN_CLIENT_REQ_FETCHREMOTE_CONNECT_TIMEOUT, "'connect timeout' is being used");
        argStream.ReadIfNextIsNumber(httpRequestOptions.uiConnectTimeoutMs, 10000);
        argStream.ReadFunction(iLuaFunction);
        argStream.ReadString(httpRequestOptions.strPostData, "");
        argStream.ReadBool(httpRequestOptions.bPostBinary, false);
        argStream.ReadLuaArguments(callbackArguments);
        argStream.ReadFunctionComplete();

        if (!argStream.HasErrors())
        {
            CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
            if (luaMain)
            {
                httpRequestOptions.bIsLegacy = true;
                CRemoteCall* pRemoteCall =
                    g_pClientGame->GetRemoteCalls()->Call(strURL, &callbackArguments, luaMain, iLuaFunction, strQueueName, httpRequestOptions);

                lua_pushuserdata(luaVM, pRemoteCall);
                return 1;
            }
        }
    }
    else
    {
        CStringMap optionsMap;

        argStream.ReadStringMap(optionsMap);
        argStream.ReadFunction(iLuaFunction);
        if (argStream.NextIsTable())
            argStream.ReadLuaArgumentsTable(callbackArguments);
        argStream.ReadFunctionComplete();

        optionsMap.ReadNumber("connectionAttempts", httpRequestOptions.uiConnectionAttempts, 10);
        optionsMap.ReadNumber("connectTimeout", httpRequestOptions.uiConnectTimeoutMs, 10000);
        optionsMap.ReadString("method", httpRequestOptions.strRequestMethod, "");
        optionsMap.ReadString("queueName", strQueueName, CALL_REMOTE_DEFAULT_QUEUE_NAME);
        optionsMap.ReadString("postData", httpRequestOptions.strPostData, "");
        optionsMap.ReadBool("postIsBinary", httpRequestOptions.bPostBinary, false);
        optionsMap.ReadNumber("maxRedirects", httpRequestOptions.uiMaxRedirects, 8);
        optionsMap.ReadString("username", httpRequestOptions.strUsername, "");
        optionsMap.ReadString("password", httpRequestOptions.strPassword, "");
        optionsMap.ReadStringMap("headers", httpRequestOptions.requestHeaders);
        optionsMap.ReadStringMap("formFields", httpRequestOptions.formFields);

        if (httpRequestOptions.formFields.empty())
            MinClientReqCheck(argStream, MIN_CLIENT_REQ_CALLREMOTE_OPTIONS_TABLE, "'options' table is being used");
        else
            MinClientReqCheck(argStream, MIN_CLIENT_REQ_CALLREMOTE_OPTIONS_FORMFIELDS, "'formFields' is being used");

        if (!argStream.HasErrors())
        {
            CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
            if (luaMain)
            {
                CRemoteCall* pRemoteCall =
                    g_pClientGame->GetRemoteCalls()->Call(strURL, &callbackArguments, luaMain, iLuaFunction, strQueueName, httpRequestOptions);

                lua_pushuserdata(luaVM, pRemoteCall);
                return 1;
            }
        }
    }

    if (argStream.HasErrors())
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

// table getRemoteRequests([resource theResource = nil])
int CLuaNetworkDefs::GetRemoteRequests(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CResource*       pResource = nullptr;
    CLuaMain*        pLuaMain = nullptr;
    int              iIndex = 0;

    argStream.ReadUserData(pResource, NULL);

    // Grab virtual machine
    if (pResource)
        pLuaMain = pResource->GetVM();

    lua_newtable(luaVM);
    for (const auto& request : g_pClientGame->GetRemoteCalls()->GetCalls())
    {
        if (!pLuaMain || request->GetVM() == pLuaMain)
        {
            lua_pushnumber(luaVM, ++iIndex);
            lua_pushuserdata(luaVM, request);
            lua_settable(luaVM, -3);
        }
    }

    return 1;
}

// table getRemoteRequestInfo(element theRequest[, number postDataLength = 0[, bool includeHeaders = false]])
int CLuaNetworkDefs::GetRemoteRequestInfo(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CValues    info, requestedHeaders;
    CRemoteCall*     pRemoteCall = nullptr;
    CResource*       pThisResource = g_pClientGame->GetResourceManager()->GetResourceFromLuaState(luaVM);
    int              iPostDataLength = 0;
    bool             bIncludeHeaders = false;

    argStream.ReadUserData(pRemoteCall);
    argStream.ReadNumber(iPostDataLength, 0);
    argStream.ReadBool(bIncludeHeaders, false);

    if (!argStream.HasErrors())
    {
        CResource* pResource = nullptr;
        if (pRemoteCall->GetVM())
            pResource = pRemoteCall->GetVM()->GetResource();

        bool bExtendedInfo = (pResource == pThisResource);

        info.Push("type");
        info.Push((pRemoteCall->IsFetch() ? "fetch" : "call"));

        // remove query_string from url when bExtendedInfo isn't set
        SString sURL = pRemoteCall->GetURL();

        if (!bExtendedInfo)
            sURL = sURL.ReplaceI("%3F", "?").Replace("#", "?").SplitLeft("?");

        info.Push("url");
        info.Push(sURL);

        info.Push("queue");
        info.Push(pRemoteCall->GetQueueName());

        info.Push("resource");

        if (pResource)
            info.Push(pResource);
        else
            info.Push(false);

        info.Push("start");
        info.Push(static_cast<double>(pRemoteCall->GetStartTime()));

        if (bExtendedInfo)
        {
            if (iPostDataLength == -1 || iPostDataLength > 0)
            {
                info.Push("postData");
                const SString& sPostData = pRemoteCall->GetOptions().strPostData;
                if (iPostDataLength > 0 && iPostDataLength < static_cast<int>(sPostData.length()))
                    info.Push(sPostData.SubStr(0, iPostDataLength));
                else
                    info.Push(sPostData);
            }

            // requested headers
            if (bIncludeHeaders)
            {
                info.Push("headers");

                for (auto const& header : pRemoteCall->GetOptions().requestHeaders)
                {
                    requestedHeaders.Push(header.first);
                    requestedHeaders.Push(header.second);
                }

                info.Push(&requestedHeaders);
            }
        }

        info.Push("method");
        info.Push((pRemoteCall->GetOptions().strRequestMethod.length() >= 1 ? pRemoteCall->GetOptions().strRequestMethod.ToUpper().c_str() : "POST"));

        info.Push("connectionAttempts");
        info.Push(pRemoteCall->GetOptions().uiConnectionAttempts);

        info.Push("connectionTimeout");
        info.Push(pRemoteCall->GetOptions().uiConnectTimeoutMs);

        // download info
        const SDownloadStatus downloadInfo = pRemoteCall->GetDownloadStatus();

        info.Push("bytesReceived");
        info.Push(downloadInfo.uiBytesReceived);

        info.Push("bytesTotal");
        info.Push(downloadInfo.uiContentLength);

        info.Push("currentAttempt");
        info.Push(downloadInfo.uiAttemptNumber);

        info.PushAsTable(luaVM);
        return 1;
    }

    lua_pushboolean(luaVM, false);
    return 1;
}

// bool abortRemoteRequest(element theRequest)
int CLuaNetworkDefs::AbortRemoteRequest(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CRemoteCall*     pRemoteCall = nullptr;

    argStream.ReadUserData(pRemoteCall);

    if (!argStream.HasErrors())
    {
        lua_pushboolean(luaVM, pRemoteCall->CancelDownload());
        g_pClientGame->GetRemoteCalls()->Remove(pRemoteCall);
        return 1;
    }
    else
    {
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }

    lua_pushboolean(luaVM, false);
    return 1;
}
