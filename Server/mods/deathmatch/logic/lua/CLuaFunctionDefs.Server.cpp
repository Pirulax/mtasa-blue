/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/lua/CLuaFunctionDefs.Server.cpp
 *  PURPOSE:     Lua special server function definitions
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#define MIN_SERVER_REQ_CALLREMOTE_QUEUE_NAME                "1.5.3-9.11270"
#define MIN_SERVER_REQ_CALLREMOTE_CONNECTION_ATTEMPTS       "1.3.0-9.04563"
#define MIN_SERVER_REQ_CALLREMOTE_CONNECT_TIMEOUT           "1.3.5"
#define MIN_SERVER_REQ_CALLREMOTE_OPTIONS_TABLE             "1.5.4-9.11342"
#define MIN_SERVER_REQ_CALLREMOTE_OPTIONS_FORMFIELDS        "1.5.4-9.11413"

int CLuaFunctionDefs::GetMaxPlayers(lua_State* luaVM)
{
    lua_pushnumber(luaVM, CStaticFunctionDefinitions::GetMaxPlayers());
    return 1;
}

int CLuaFunctionDefs::SetMaxPlayers(lua_State* luaVM)
{
    unsigned int uiMaxPlayers;

    CScriptArgReader argStream(luaVM);
    argStream.ReadNumber(uiMaxPlayers);

    if (!argStream.HasErrors())
    {
        lua_pushboolean(luaVM, CStaticFunctionDefinitions::SetMaxPlayers(uiMaxPlayers));
        return 1;
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::OutputChatBox(lua_State* luaVM)
{
    // bool outputChatBox ( string text [, element/table visibleTo=getRootElement(), int r=231, int g=217, int b=176, bool colorCoded=false ] )
    SString               ssChat;
    std::vector<CPlayer*> sendList;
    CElement*             pElement = nullptr;
    bool                  bColorCoded;
    // Default
    unsigned char ucRed = 231;
    unsigned char ucGreen = 217;
    unsigned char ucBlue = 176;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(ssChat);

    if (argStream.NextIsTable())
    {
        argStream.ReadUserDataTable(sendList);
    }
    else
    {
        argStream.ReadUserData(pElement, m_pRootElement);
    }

    if (argStream.NextIsNumber() && argStream.NextIsNumber(1) && argStream.NextIsNumber(2))
    {
        argStream.ReadNumber(ucRed);
        argStream.ReadNumber(ucGreen);
        argStream.ReadNumber(ucBlue);
    }
    else
        argStream.Skip(3);

    argStream.ReadBool(bColorCoded, false);

    if (!argStream.HasErrors())
    {
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            if (pElement)
            {
                if (IS_TEAM(pElement))
                {
                    CTeam* pTeam = static_cast<CTeam*>(pElement);
                    for (auto iter = pTeam->PlayersBegin(); iter != pTeam->PlayersEnd(); iter++)
                    {
                        sendList.push_back(*iter);
                    }
                }
                else
                {
                    CStaticFunctionDefinitions::OutputChatBox((const char*)ssChat, pElement, ucRed, ucGreen, ucBlue, bColorCoded, pLuaMain);
                    lua_pushboolean(luaVM, true);
                    return 1;
                }
            }

            if (sendList.size() > 0)
            {
                CStaticFunctionDefinitions::OutputChatBox((const char*)ssChat, sendList, ucRed, ucGreen, ucBlue, bColorCoded);
                lua_pushboolean(luaVM, true);
                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::OOP_OutputChatBox(lua_State* luaVM)
{
    // bool Player:outputChat ( string text [, int r=231, int g=217, int b=176, bool colorCoded=false ] )
    CElement* pElement;
    SString   strText;
    uchar     ucRed = 231;
    uchar     ucGreen = 217;
    uchar     ucBlue = 176;
    bool      bColorCoded;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(pElement);
    argStream.ReadString(strText);

    if (argStream.NextIsNumber(0) && argStream.NextIsNumber(1) && argStream.NextIsNumber(2))
    {
        argStream.ReadNumber(ucRed);
        argStream.ReadNumber(ucGreen);
        argStream.ReadNumber(ucBlue);
    }
    else
        argStream.Skip(3);

    argStream.ReadBool(bColorCoded, false);

    if (!argStream.HasErrors())
    {
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            CStaticFunctionDefinitions::OutputChatBox(strText, pElement, ucRed, ucGreen, ucBlue, bColorCoded, pLuaMain);
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::ClearChatBox(lua_State* luaVM)
{
    CElement* pElement;

    CScriptArgReader argStream(luaVM);

    argStream.ReadUserData(pElement, m_pRootElement);

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::ClearChatBox(pElement))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::OutputConsole(lua_State* luaVM)
{
    SString   strMessage;
    CElement* pElement;

    CScriptArgReader argStream(luaVM);

    argStream.ReadString(strMessage);
    argStream.ReadUserData(pElement, m_pRootElement);

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::OutputConsole(strMessage, pElement))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::OutputDebugString(lua_State* luaVM)
{
    SString       strMessage;
    unsigned int  uiLevel;
    unsigned char ucR, ucG, ucB;

    CScriptArgReader argStream(luaVM);
    argStream.ReadAnyAsString(strMessage);
    argStream.ReadNumber(uiLevel, 3);

    if (uiLevel == 0 || uiLevel == 4)
    {
        argStream.ReadNumber(ucR, 255);
        argStream.ReadNumber(ucG, 255);
        argStream.ReadNumber(ucB, 255);
    }

    if (!argStream.HasErrors())
    {
        if (uiLevel > 4)
        {
            m_pScriptDebugging->LogWarning(luaVM, "Bad level argument sent to %s (0-4)", lua_tostring(luaVM, lua_upvalueindex(1)));

            lua_pushboolean(luaVM, false);
            return 1;
        }

        if (uiLevel == 1)
        {
            m_pScriptDebugging->LogError(luaVM, "%s", strMessage.c_str());
        }
        else if (uiLevel == 2)
        {
            m_pScriptDebugging->LogWarning(luaVM, "%s", strMessage.c_str());
        }
        else if (uiLevel == 3)
        {
            m_pScriptDebugging->LogInformation(luaVM, "%s", strMessage.c_str());
        }
        else if (uiLevel == 4)
        {
            m_pScriptDebugging->LogCustom(luaVM, ucR, ucG, ucB, "%s", strMessage.c_str());
        }
        else if (uiLevel == 0)
        {
            m_pScriptDebugging->LogDebug(luaVM, ucR, ucG, ucB, "%s", strMessage.c_str());
        }
        lua_pushboolean(luaVM, true);
        return 1;
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::AddCommandHandler(lua_State* luaVM)
{
    //  bool addCommandHandler ( string commandName, function handlerFunction, [bool restricted = false, bool caseSensitive = true] )
    SString         strKey;
    CLuaFunctionRef iLuaFunction;
    bool            bRestricted;
    bool            bCaseSensitive;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);
    argStream.ReadFunction(iLuaFunction);
    argStream.ReadBool(bRestricted, false);
    argStream.ReadBool(bCaseSensitive, true);
    argStream.ReadFunctionComplete();

    if (!argStream.HasErrors())
    {
        // Grab our VM
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            // Add them to our list over command handlers
            if (m_pRegisteredCommands->AddCommand(pLuaMain, strKey, iLuaFunction, bRestricted, bCaseSensitive))
            {
                lua_pushboolean(luaVM, true);
                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::RemoveCommandHandler(lua_State* luaVM)
{
    //  bool removeCommandHandler ( string commandName [, function handler] )
    SString         strKey;
    CLuaFunctionRef iLuaFunction;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);
    argStream.ReadFunction(iLuaFunction, LUA_REFNIL);
    argStream.ReadFunctionComplete();

    if (!argStream.HasErrors())
    {
        // Grab our VM
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            // Remove it from our list
            if (m_pRegisteredCommands->RemoveCommand(pLuaMain, strKey, iLuaFunction))
            {
                lua_pushboolean(luaVM, true);
                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::ExecuteCommandHandler(lua_State* luaVM)
{
    //  bool executeCommandHandler ( string commandName, player thePlayer, [ string args ] )
    SString   strKey;
    CElement* pElement;
    SString   strArgs;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);
    argStream.ReadUserData(pElement);
    argStream.ReadString(strArgs, "");

    if (!argStream.HasErrors())
    {
        // Grab our VM
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            CClient* pClient = NULL;
            if (pElement->GetType() == CElement::PLAYER)
                pClient = static_cast<CClient*>(static_cast<CPlayer*>(pElement));

            if (pClient)
            {
                // Call it
                if (m_pRegisteredCommands->ProcessCommand(strKey, strArgs, pClient))
                {
                    lua_pushboolean(luaVM, true);
                    return 1;
                }
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetCommandHandlers(lua_State* luaVM)
{
    // table getCommandHandlers ( [ resource sourceResource ] );
    CResource* pResource = nullptr;

    CScriptArgReader argStream(luaVM);

    if (!argStream.NextIsNil() && !argStream.NextIsNone())
        argStream.ReadUserData(pResource);

    if (argStream.HasErrors())
    {
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
        lua_pushnil(luaVM);
        return 1;
    }

    if (pResource)
    {
        CLuaMain* pLuaMain = pResource->GetVirtualMachine();

        if (pLuaMain)
            m_pRegisteredCommands->GetCommands(luaVM, pLuaMain);
        else
            lua_newtable(luaVM);
    }
    else
    {
        m_pRegisteredCommands->GetCommands(luaVM);
    }

    return 1;
}

int CLuaFunctionDefs::OutputServerLog(lua_State* luaVM)
{
    SString strMessage;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strMessage);

    if (!argStream.HasErrors())
    {
        // Print it
        CLogger::LogPrintf(LOGLEVEL_MEDIUM, "%s\n", strMessage.c_str());
        lua_pushboolean(luaVM, true);

        return 1;
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetServerPort(lua_State* luaVM)
{
    lua_pushnumber(luaVM, g_pGame->GetConfig()->GetServerPort());
    return 1;
}

int CLuaFunctionDefs::Set(lua_State* luaVM)
{
    SString       strSetting;
    CLuaArguments Args;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strSetting);
    argStream.ReadLuaArguments(Args);

    if (!argStream.HasErrors())
    {
        CResource* pResource = m_pLuaManager->GetVirtualMachineResource(luaVM);
        if (pResource)
        {
            std::string strResourceName = pResource->GetName();
            std::string strJSON;
            Args.WriteToJSONString(strJSON);

            if (g_pGame->GetSettings()->Set(strResourceName.c_str(), strSetting.c_str(), strJSON.c_str()))
            {
                lua_pushboolean(luaVM, true);
                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

/* #define PUSH_SETTING(x,buf) \
pAttributes = &(x->GetAttributes ()); \
Args.PushString ( pAttributes->Find ( "name" )->GetValue ().c_str () ); \
buf = const_cast < char* > ( pAttributes->Find ( "value" )->GetValue ().c_str () ); \
if ( !Args.ReadFromJSONString ( buf ) ) { \
Args.PushString ( buf ); \
}
*/

int CLuaFunctionDefs::Get(lua_State* luaVM)
{
    SString       strSetting;
    CLuaArguments Args;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strSetting);

    if (!argStream.HasErrors())
    {
        CResource* pResource = m_pLuaManager->GetVirtualMachineResource(luaVM);
        if (pResource)
        {
            unsigned int uiIndex = 0;
            bool         bDeleteNode;

            // Extract attribute name if setting to be gotten has three parts i.e. resname.settingname.attributename
            SString         strAttribute = "value";
            vector<SString> Result;
            strSetting.Split(".", Result);
            if (Result.size() == 3 && Result[2].length())
            {
                strAttribute = Result[2];
            }

            // Get the setting
            CXMLNode *pSubNode, *pNode = g_pGame->GetSettings()->Get(pResource->GetName().c_str(), strSetting.c_str(), bDeleteNode);

            // Only proceed if we have a valid node
            if (pNode)
            {
                // Argument count
                unsigned int uiArgCount = 1;

                // See if we need to return a table with single or multiple entries
                if (pNode->GetSubNodeCount() == 0)
                {
                    // See if required attribute exists
                    CXMLAttribute* pAttribute = pNode->GetAttributes().Find(strAttribute.c_str());
                    if (!pAttribute)
                    {
                        if (bDeleteNode)
                            delete pNode;
                        lua_pushboolean(luaVM, false);
                        return 1;
                    }
                    // We only have a single entry for a specific setting, so output a string
                    const std::string& strDataValue = pAttribute->GetValue();
                    if (!Args.ReadFromJSONString(strDataValue.c_str()))
                    {
                        // No valid JSON? Parse as plain text
                        Args.PushString(strDataValue);
                    }

                    Args.PushArguments(luaVM);
                    uiArgCount = Args.Count();

                    /* Don't output a table because although it is more consistent with the multiple values output below,
                    ** due to lua's implementation of associative arrays (assuming we use the "setting-name", "value" key-value pairs)
                    ** it would require the scripter to walk through an array that only has a single entry which is a Bad Thing, performance wise.
                    **
                    PUSH_SETTING ( pNode );
                    Args.PushAsTable ( luaVM );
                    **/
                }
                else
                {
                    // We need to return multiply entries, so push all subnodes
                    while ((pSubNode = pNode->FindSubNode("setting", uiIndex++)))
                    {
                        CXMLAttributes& attributes = pSubNode->GetAttributes();
                        Args.PushString(attributes.Find("name")->GetValue());
                        const std::string& strDataValue = attributes.Find("value")->GetValue();
                        if (!Args.ReadFromJSONString(strDataValue.c_str()))
                        {
                            Args.PushString(strDataValue);
                        }
                    }
                    // Push a table and return
                    Args.PushAsTable(luaVM);
                }

                // Check if we have to delete the node
                if (bDeleteNode)
                    delete pNode;

                return uiArgCount;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

// Call a function on a remote server
int CLuaFunctionDefs::CallRemote(lua_State* luaVM)
{
    /* Determine if the argument stream is either a remote-server resource call or a web call:
     * a) element callRemote ( string host [, string queueName ][, int connectionAttempts = 10, int connectTimeout = 10000 ], string resourceName, string
     * functionName, callback callbackFunction, [ arguments... ] ) b) bool callRemote ( string URL [, string queueName ][, int connectionAttempts = 10, int
     * connectTimeout = 10000 ], callback callbackFunction, [ arguments... ] )
     */
    CScriptArgReader argStream(luaVM);
    SString          strHost;
    SString          strQueueName = CALL_REMOTE_DEFAULT_QUEUE_NAME;

    argStream.ReadString(strHost);

    /* Find out if the next parameter is the 'queueName' argument
     * 1) string queueName, int connectionAttempts, ...
     * 2) string queueName, callback callbackFunction, ...
     * 3) string queueName, string resourceName, string functionName, ...
     */
    if (argStream.NextIsString() && (argStream.NextIsNumber(1) || argStream.NextIsFunction(1) || (argStream.NextIsString(1) && argStream.NextIsString(2))))
    {
        MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_QUEUE_NAME, "'queue name' is being used");
        argStream.ReadString(strQueueName, CALL_REMOTE_DEFAULT_QUEUE_NAME);
    }

    // Read connectionAttempts and connectTimeout arguments if given
    uint uiConnectionAttempts = 10U;
    uint uiConnectTimeoutMs = 10000U;

    if (argStream.NextIsNumber())
        MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_CONNECTION_ATTEMPTS, "'connection attempts' is being used");
    argStream.ReadIfNextIsNumber(uiConnectionAttempts, 10U);

    if (argStream.NextIsNumber())
        MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_CONNECT_TIMEOUT, "'connect timeout' is being used");
    argStream.ReadIfNextIsNumber(uiConnectTimeoutMs, 10000U);

    // Continue with either call type a) or b)
    CLuaFunctionRef iLuaFunction;
    CLuaArguments   args;

    if (argStream.NextIsString() && argStream.NextIsString(1) && argStream.NextIsFunction(2))
    {
        SString strResourceName;
        SString strFunctionName;
        argStream.ReadString(strResourceName);
        argStream.ReadString(strFunctionName);
        argStream.ReadFunction(iLuaFunction);
        argStream.ReadLuaArguments(args);
        argStream.ReadFunctionComplete();

        if (!argStream.HasErrors())
        {
            CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
            if (luaMain)
            {
                CRemoteCall* pRemoteCall = g_pGame->GetRemoteCalls()->Call(strHost, strResourceName, strFunctionName, &args, luaMain, iLuaFunction,
                                                                           strQueueName, uiConnectionAttempts, uiConnectTimeoutMs);

                lua_pushuserdata(luaVM, pRemoteCall);
                return 1;
            }
        }
    }
    else if (argStream.NextIsFunction())
    {
        argStream.ReadFunction(iLuaFunction);
        argStream.ReadLuaArguments(args);
        argStream.ReadFunctionComplete();

        if (!argStream.HasErrors())
        {
            CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
            if (luaMain)
            {
                CRemoteCall* pRemoteCall =
                    g_pGame->GetRemoteCalls()->Call(strHost, &args, luaMain, iLuaFunction, strQueueName, uiConnectionAttempts, uiConnectTimeoutMs);

                lua_pushuserdata(luaVM, pRemoteCall);
                return 1;
            }
        }
    }
    else
    {
        argStream.SetCustomWarning("Unrecognized argument list for callRemote: bad arguments or missing arguments");
    }

    if (argStream.HasErrors())
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

// Call a function on a remote server
int CLuaFunctionDefs::FetchRemote(lua_State* luaVM)
{
    //  element fetchRemote ( string URL [, string queueName ][, int connectionAttempts = 10, int connectTimeout = 10000 ], callback callbackFunction, [ string
    //  postData, bool bPostBinary, arguments... ] ) bool fetchRemote ( string URL [, table options ], callback callbackFunction[, table callbackArguments ] )
    CScriptArgReader    argStream(luaVM);
    SString             strURL;
    SString             strQueueName;
    SHttpRequestOptions httpRequestOptions;
    CLuaFunctionRef     iLuaFunction;
    CLuaArguments       callbackArguments;

    argStream.ReadString(strURL);
    if (!argStream.NextIsTable())
    {
        if (argStream.NextIsString())
            MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_QUEUE_NAME, "'queue name' is being used");
        argStream.ReadIfNextIsString(strQueueName, CALL_REMOTE_DEFAULT_QUEUE_NAME);
        if (argStream.NextIsNumber())
            MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_CONNECTION_ATTEMPTS, "'connection attempts' is being used");
        argStream.ReadIfNextIsNumber(httpRequestOptions.uiConnectionAttempts, 10);
        if (argStream.NextIsNumber())
            MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_CONNECT_TIMEOUT, "'connect timeout' is being used");
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
                CRemoteCall* pRemoteCall = g_pGame->GetRemoteCalls()->Call(strURL, &callbackArguments, luaMain, iLuaFunction, strQueueName, httpRequestOptions);

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
            MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_OPTIONS_TABLE, "'options' table is being used");
        else
            MinServerReqCheck(argStream, MIN_SERVER_REQ_CALLREMOTE_OPTIONS_FORMFIELDS, "'formFields' is being used");

        if (!argStream.HasErrors())
        {
            CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
            if (luaMain)
            {
                CRemoteCall* pRemoteCall = g_pGame->GetRemoteCalls()->Call(strURL, &callbackArguments, luaMain, iLuaFunction, strQueueName, httpRequestOptions);

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
int CLuaFunctionDefs::GetRemoteRequests(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CResource*       pResource = nullptr;
    CLuaMain*        pLuaMain = nullptr;
    int              iIndex = 0;

    argStream.ReadUserData(pResource, NULL);

    if (pResource)
        pLuaMain = pResource->GetVirtualMachine();

    if (!argStream.HasErrors())
    {
        lua_newtable(luaVM);

        for (const auto& request : g_pGame->GetRemoteCalls()->GetCalls())
        {
            if (!pResource || request->GetVM() == pLuaMain)
            {
                lua_pushnumber(luaVM, ++iIndex);
                lua_pushuserdata(luaVM, request);
                lua_settable(luaVM, -3);
            }
        }

        return 1;
    }
    else
    {
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }

    lua_pushboolean(luaVM, false);
    return 1;
}

// table getRemoteRequestInfo(element theRequest[, number postDataLength = 0[, bool includeHeaders = false]])
int CLuaFunctionDefs::GetRemoteRequestInfo(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CLuaArguments    info, requestedHeaders;
    CRemoteCall*     pRemoteCall = nullptr;
    CResource*       pThisResource = m_pResourceManager->GetResourceFromLuaState(luaVM);
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

        bool bExtendedInfo = false;

        // only extend informations when the called resource is the same OR has "general.fullRemoteRequestInfo" acl right
        if (pThisResource == pResource ||
            m_pACLManager->CanObjectUseRight(pThisResource->GetName().c_str(), CAccessControlListGroupObject::OBJECT_TYPE_RESOURCE, "fullRemoteRequestInfo",
                                             CAccessControlListRight::RIGHT_TYPE_GENERAL, false))
        {
            bExtendedInfo = true;
        }

        info.PushString("type");
        info.PushString((pRemoteCall->IsFetch() ? "fetch" : "call"));

        // remove query_string from url when bExtendedInfo isn't set
        SString sURL = pRemoteCall->GetURL();

        if (!bExtendedInfo)
            sURL = sURL.ReplaceI("%3F", "?").Replace("#", "?").SplitLeft("?");

        info.PushString("url");
        info.PushString(sURL);

        info.PushString("queue");
        info.PushString(pRemoteCall->GetQueueName());

        info.PushString("resource");

        if (pResource)
            info.PushResource(pResource);
        else
            info.PushBoolean(false);

        info.PushString("start");
        info.PushNumber(static_cast<double>(pRemoteCall->GetStartTime()));

        if (bExtendedInfo)
        {
            if (iPostDataLength == -1 || iPostDataLength > 0)
            {
                info.PushString("postData");
                const SString& sPostData = pRemoteCall->GetOptions().strPostData;
                if (iPostDataLength > 0 && iPostDataLength < static_cast<int>(sPostData.length()))
                    info.PushString(sPostData.SubStr(0, iPostDataLength));
                else
                    info.PushString(sPostData);
            }

            // requested headers
            if (bIncludeHeaders)
            {
                info.PushString("headers");

                for (auto const& header : pRemoteCall->GetOptions().requestHeaders)
                {
                    requestedHeaders.PushString(header.first);
                    requestedHeaders.PushString(header.second);
                }

                info.PushTable(&requestedHeaders);
            }
        }

        info.PushString("method");
        info.PushString((pRemoteCall->GetOptions().strRequestMethod.length() >= 1 ? pRemoteCall->GetOptions().strRequestMethod.ToUpper().c_str() : "POST"));

        info.PushString("connectionAttempts");
        info.PushNumber(pRemoteCall->GetOptions().uiConnectionAttempts);

        info.PushString("connectionTimeout");
        info.PushNumber(pRemoteCall->GetOptions().uiConnectTimeoutMs);

        // download info
        SDownloadStatus downloadInfo = pRemoteCall->GetDownloadStatus();

        info.PushString("bytesReceived");
        info.PushNumber(downloadInfo.uiBytesReceived);

        info.PushString("bytesTotal");
        info.PushNumber(downloadInfo.uiContentLength);

        info.PushString("currentAttempt");
        info.PushNumber(downloadInfo.uiAttemptNumber);

        info.PushAsTable(luaVM);
        return 1;
    }

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::AbortRemoteRequest(lua_State* luaVM)
{
    CScriptArgReader argStream(luaVM);
    CRemoteCall*     pRemoteCall = nullptr;

    argStream.ReadUserData(pRemoteCall);

    if (!argStream.HasErrors())
    {
        lua_pushboolean(luaVM, pRemoteCall->CancelDownload());
        g_pGame->GetRemoteCalls()->Remove(pRemoteCall);
        return 1;
    }
    else
    {
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetServerName(lua_State* luaVM)
{
    lua_pushstring(luaVM, g_pGame->GetConfig()->GetServerName().c_str());
    return 1;
}

int CLuaFunctionDefs::GetServerHttpPort(lua_State* luaVM)
{
    lua_pushnumber(luaVM, g_pGame->GetConfig()->GetHTTPPort());
    return 1;
}

int CLuaFunctionDefs::GetServerIP(lua_State* luaVM)
{
    lua_pushstring(luaVM, "moo");
    return 1;
}

int CLuaFunctionDefs::GetServerPassword(lua_State* luaVM)
{
    // We have a password? Return it.
    if (g_pGame->GetConfig()->HasPassword())
    {
        // Return it
        lua_pushstring(luaVM, g_pGame->GetConfig()->GetPassword().c_str());
        return 1;
    }

    // Otherwize return nil for no password
    lua_pushnil(luaVM);
    return 1;
}

int CLuaFunctionDefs::SetServerPassword(lua_State* luaVM)
{
    //  bool setServerPassword ( [ string password ] )
    SString strPassword;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strPassword, "");

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::SetServerPassword(strPassword, true))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
        else
            argStream.SetCustomError("password must be shorter than 32 chars and just contain visible characters");
    }
    if (argStream.HasErrors())
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetServerConfigSetting(lua_State* luaVM)
{
    //  string getServerConfigSetting ( string name )
    SString strName;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strName);

    if (!argStream.HasErrors())
    {
        SString strValue;
        // Try as single setting
        if (g_pGame->GetConfig()->GetSetting(strName, strValue))
        {
            lua_pushstring(luaVM, strValue);
            return 1;
        }
        // Try as multiple setting
        CLuaArguments result;
        if (g_pGame->GetConfig()->GetSettingTable(strName, &result))
        {
            result.PushAsTable(luaVM);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::SetServerConfigSetting(lua_State* luaVM)
{
    //  bool setServerConfigSetting ( string name, string value [, bool save = false ] )
    SString strName;
    SString strValue;
    bool    bSave;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strName);
    argStream.ReadString(strValue);
    argStream.ReadBool(bSave, false);

    if (!argStream.HasErrors())
    {
        if (g_pGame->GetConfig()->SetSetting(strName, strValue, bSave))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::shutdown(lua_State* luaVM)
{
    SString strReason;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strReason, "No reason specified");

    if (!argStream.HasErrors())
    {
        // Get the VM
        CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (pLuaMain)
        {
            // Get the resource
            CResource* pResource = pLuaMain->GetResource();
            if (pResource)
            {
                // Log it
                CLogger::LogPrintf("Server shutdown as requested by resource %s (%s)\n", pResource->GetName().c_str(), *strReason);

                // Shut it down
                g_pGame->SetIsFinished(true);

                // Success
                lua_pushboolean(luaVM, true);
                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    // Fail
    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetMapName(lua_State* luaVM)
{
    // Grab ASE
    ASE* pASE = ASE::GetInstance();
    if (pASE)
    {
        // Grab the mapname string
        const char* szMapName = pASE->GetMapName();
        if (szMapName[0] != 0)
        {
            // Return the gametype string excluding our prefix
            lua_pushstring(luaVM, szMapName);
            return 1;
        }
    }

    // No game type
    lua_pushnil(luaVM);
    return 1;
}

int CLuaFunctionDefs::GetGameType(lua_State* luaVM)
{
    // Grab ASE
    ASE* pASE = ASE::GetInstance();
    if (pASE)
    {
        // Grab the gametype string.
        const char* szGameType = pASE->GetGameType();

        // Return the gametype string if it's not "MTA:SA"
        if (strcmp(szGameType, GAME_TYPE_STRING))
        {
            lua_pushstring(luaVM, szGameType);
            return 1;
        }
    }

    // No game type
    lua_pushnil(luaVM);
    return 1;
}

int CLuaFunctionDefs::SetGameType(lua_State* luaVM)
{
    //  bool setGameType ( string gameType )
    SString strGameType;

    CScriptArgReader argStream(luaVM);
    argStream.ReadIfNextIsString(strGameType, "");            // Default to empty for backward compat with previous implementation

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::SetGameType(strGameType))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    // Failed
    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::SetMapName(lua_State* luaVM)
{
    //  bool setMapName ( string mapName )
    SString strMapName;

    CScriptArgReader argStream(luaVM);
    argStream.ReadIfNextIsString(strMapName, "");            // Default to empty for backward compat with previous implementation

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::SetMapName(strMapName))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetRuleValue(lua_State* luaVM)
{
    //  string getRuleValue ( string key )
    SString strKey;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);

    if (!argStream.HasErrors())
    {
        const char* szRule = CStaticFunctionDefinitions::GetRuleValue(strKey);
        if (szRule)
        {
            lua_pushstring(luaVM, szRule);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::SetRuleValue(lua_State* luaVM)
{
    //  bool setRuleValue ( string key, string value )
    SString strKey;
    SString strValue;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);
    argStream.ReadString(strValue);

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::SetRuleValue(strKey, strValue))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::RemoveRuleValue(lua_State* luaVM)
{
    //  bool removeRuleValue ( string key )
    SString strKey;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strKey);

    if (!argStream.HasErrors())
    {
        if (CStaticFunctionDefinitions::RemoveRuleValue(strKey))
        {
            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetNetworkUsageData(lua_State* luaVM)
{
    SPacketStat m_PacketStats[2][256];
    memcpy(m_PacketStats, g_pNetServer->GetPacketStats(), sizeof(m_PacketStats));

    lua_createtable(luaVM, 0, 2);

    lua_pushstring(luaVM, "in");
    lua_createtable(luaVM, 0, 2);
    {
        lua_pushstring(luaVM, "bits");
        lua_createtable(luaVM, 255, 1);
        for (unsigned int i = 0; i < 256; ++i)
        {
            const SPacketStat& statIn = m_PacketStats[CNetServer::STATS_INCOMING_TRAFFIC][i];
            lua_pushnumber(luaVM, statIn.iTotalBytes * 8);
            lua_rawseti(luaVM, -2, i);
        }
        lua_rawset(luaVM, -3);

        lua_pushstring(luaVM, "count");
        lua_createtable(luaVM, 255, 1);
        for (unsigned int i = 0; i < 256; ++i)
        {
            const SPacketStat& statIn = m_PacketStats[CNetServer::STATS_INCOMING_TRAFFIC][i];
            lua_pushnumber(luaVM, statIn.iCount);
            lua_rawseti(luaVM, -2, i);
        }
        lua_rawset(luaVM, -3);
    }
    lua_rawset(luaVM, -3);

    lua_pushstring(luaVM, "out");
    lua_createtable(luaVM, 0, 2);
    {
        lua_pushstring(luaVM, "bits");
        lua_createtable(luaVM, 255, 1);
        for (unsigned int i = 0; i < 256; ++i)
        {
            const SPacketStat& statOut = m_PacketStats[CNetServer::STATS_OUTGOING_TRAFFIC][i];
            lua_pushnumber(luaVM, statOut.iTotalBytes * 8);
            lua_rawseti(luaVM, -2, i);
        }
        lua_rawset(luaVM, -3);

        lua_pushstring(luaVM, "count");
        lua_createtable(luaVM, 255, 1);
        for (unsigned int i = 0; i < 256; ++i)
        {
            const SPacketStat& statOut = m_PacketStats[CNetServer::STATS_OUTGOING_TRAFFIC][i];
            lua_pushnumber(luaVM, statOut.iCount);
            lua_rawseti(luaVM, -2, i);
        }
        lua_rawset(luaVM, -3);
    }
    lua_rawset(luaVM, -3);

    return 1;
}

int CLuaFunctionDefs::GetNetworkStats(lua_State* luaVM)
{
    //  table getNetworkStats ( [element player] )
    CPlayer* pPlayer;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(pPlayer, NULL);

    if (!argStream.HasErrors())
    {
        NetServerPlayerID PlayerID = pPlayer ? pPlayer->GetSocket() : NetServerPlayerID();
        NetStatistics     stats;
        if (g_pNetServer->GetNetworkStatistics(&stats, PlayerID))
        {
            lua_createtable(luaVM, 0, 11);

            lua_pushstring(luaVM, "bytesReceived");
            lua_pushnumber(luaVM, static_cast<double>(stats.bytesReceived));
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "bytesSent");
            lua_pushnumber(luaVM, static_cast<double>(stats.bytesSent));
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "packetsReceived");
            lua_pushnumber(luaVM, stats.packetsReceived);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "packetsSent");
            lua_pushnumber(luaVM, stats.packetsSent);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "packetlossTotal");
            lua_pushnumber(luaVM, stats.packetlossTotal);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "packetlossLastSecond");
            lua_pushnumber(luaVM, stats.packetlossLastSecond);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "messagesInSendBuffer");
            lua_pushnumber(luaVM, stats.messagesInSendBuffer);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "messagesInResendBuffer");
            lua_pushnumber(luaVM, stats.messagesInResendBuffer);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "isLimitedByCongestionControl");
            lua_pushnumber(luaVM, stats.isLimitedByCongestionControl ? 1 : 0);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "isLimitedByOutgoingBandwidthLimit");
            lua_pushnumber(luaVM, stats.isLimitedByOutgoingBandwidthLimit ? 1 : 0);
            lua_settable(luaVM, -3);

            lua_pushstring(luaVM, "encryptionStatus");
            lua_pushnumber(luaVM, 1);
            lua_settable(luaVM, -3);

            return 1;
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetVersion(lua_State* luaVM)
{
    lua_createtable(luaVM, 0, 8);

    lua_pushstring(luaVM, "number");
    lua_pushnumber(luaVM, CStaticFunctionDefinitions::GetVersion());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "mta");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetVersionString());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "name");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetVersionName());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "netcode");
    lua_pushnumber(luaVM, CStaticFunctionDefinitions::GetNetcodeVersion());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "os");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetOperatingSystemName());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "type");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetVersionBuildType());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "tag");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetVersionBuildTag());
    lua_settable(luaVM, -3);

    lua_pushstring(luaVM, "sortable");
    lua_pushstring(luaVM, CStaticFunctionDefinitions::GetVersionSortable());
    lua_settable(luaVM, -3);

    return 1;
}

int CLuaFunctionDefs::GetModuleInfo(lua_State* luaVM)
{
    SString strModuleName;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strModuleName);

    if (!argStream.HasErrors())
    {
        std::list<CLuaModule*> modules = m_pLuaModuleManager->GetLoadedModules();
        for (const auto mod : modules)
        {
            if (mod->_GetName() == strModuleName)
            {
                lua_newtable(luaVM);

                lua_pushstring(luaVM, "name");
                lua_pushstring(luaVM, mod->_GetFunctions().szModuleName);
                lua_settable(luaVM, -3);

                lua_pushstring(luaVM, "author");
                lua_pushstring(luaVM, mod->_GetFunctions().szAuthor);
                lua_settable(luaVM, -3);

                lua_pushstring(luaVM, "version");
                SString strVersion("%.2f", mod->_GetFunctions().fVersion);
                lua_pushstring(luaVM, strVersion);
                lua_settable(luaVM, -3);

                return 1;
            }
        }
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetModules(lua_State* luaVM)
{
    lua_newtable(luaVM);
    list<CLuaModule*>           lua_LoadedModules = m_pLuaModuleManager->GetLoadedModules();
    list<CLuaModule*>::iterator iter = lua_LoadedModules.begin();
    unsigned int                uiIndex = 1;
    for (; iter != lua_LoadedModules.end(); ++iter)
    {
        lua_pushnumber(luaVM, uiIndex++);
        lua_pushstring(luaVM, (*iter)->_GetFunctions().szFileName);
        lua_settable(luaVM, -3);
    }
    return 1;
}

int CLuaFunctionDefs::GetPerformanceStats(lua_State* luaVM)
{
    SString strCategory, strOptions, strFilter;

    CScriptArgReader argStream(luaVM);
    argStream.ReadString(strCategory);
    argStream.ReadString(strOptions, "");
    argStream.ReadString(strFilter, "");

    if (!argStream.HasErrors())
    {
        CPerfStatResult Result;
        CPerfStatManager::GetSingleton()->GetStats(&Result, strCategory, strOptions, strFilter);

        lua_newtable(luaVM);
        for (int c = 0; c < Result.ColumnCount(); c++)
        {
            const SString& name = Result.ColumnName(c);
            lua_pushnumber(luaVM, c + 1);            // row index number (starting at 1, not 0)
            lua_pushlstring(luaVM, name.c_str(), name.length());
            lua_settable(luaVM, -3);
        }

        lua_newtable(luaVM);
        for (int r = 0; r < Result.RowCount(); r++)
        {
            lua_newtable(luaVM);                     // new table
            lua_pushnumber(luaVM, r + 1);            // row index number (starting at 1, not 0)
            lua_pushvalue(luaVM, -2);                // value
            lua_settable(luaVM, -4);                 // refer to the top level table

            for (int c = 0; c < Result.ColumnCount(); c++)
            {
                SString& cell = Result.Data(c, r);
                lua_pushnumber(luaVM, c + 1);
                lua_pushlstring(luaVM, cell.c_str(), cell.length());
                lua_settable(luaVM, -3);
            }
            lua_pop(luaVM, 1);            // pop the inner table
        }
        return 2;
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::SetDevelopmentMode(lua_State* luaVM)
{
    // bool setDevelopmentMode ( bool enable )
    bool enable;

    CScriptArgReader argStream(luaVM);
    argStream.ReadBool(enable);

    if (!argStream.HasErrors())
    {
        g_pGame->SetDevelopmentMode(enable);
        lua_pushboolean(luaVM, true);
        return 1;
    }
    else
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaFunctionDefs::GetDevelopmentMode(lua_State* luaVM)
{
    // bool getDevelopmentMode ()
    lua_pushboolean(luaVM, g_pGame->GetDevelopmentMode());
    return 1;
}
