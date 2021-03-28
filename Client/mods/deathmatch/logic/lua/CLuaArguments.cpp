/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/lua/CLuaArguments.cpp
 *  PURPOSE:     Lua arguments manager class
 *
 *****************************************************************************/

#include "StdInc.h"
#define DECLARE_PROFILER_SECTION_CLuaArguments
#include "profiler/SharedUtil.Profiler.h"

using namespace std;

#ifndef VERIFY_ENTITY
#define VERIFY_ENTITY(entity) (CStaticFunctionDefinitions::GetRootElement()->IsMyChild(entity,true)&&!entity->IsBeingDeleted())
#endif

extern CClientGame* g_pClientGame;

CValues::CValues(NetBitStreamInterface& bitStream, std::vector<CValues*>* pKnownTables)
{
    Read(bitStream, pKnownTables);
}

CValues::CValues(const CValues& Arguments, CFastHashMap<CValues*, CValues*>* pKnownTables)
{
    // Copy all the arguments
    CopyRecursive(Arguments, pKnownTables);
}

CValue* CValues::operator[](const unsigned int uiPosition) const
{
    if (uiPosition < m_Arguments.size())
        return m_Arguments.at(uiPosition);
    return NULL;
}

// Slow if used with a constructor as it does a copy twice
const CValues& CValues::operator=(const CValues& Arguments)
{
    CopyRecursive(Arguments);

    // Return the given reference allowing for chaining
    return Arguments;
}

void CValues::CopyRecursive(const CValues& Arguments, CFastHashMap<CValues*, CValues*>* pKnownTables)
{
    // Clear our previous list if any
    DeleteArguments();

    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new CFastHashMap<CValues*, CValues*>();
        bKnownTablesCreated = true;
    }

    pKnownTables->insert(std::make_pair((CValues*)&Arguments, (CValues*)this));

    // Copy all the arguments
    vector<CValue*>::const_iterator iter = Arguments.m_Arguments.begin();
    for (; iter != Arguments.m_Arguments.end(); iter++)
    {
        CValue* pArgument = new CValue(**iter, pKnownTables);
        m_Arguments.push_back(pArgument);
    }

    if (bKnownTablesCreated)
        delete pKnownTables;
}

void CValues::ReadAll(lua_State* luaVM, signed int uiIndexBegin)
{
    // Delete the previous arguments if any
    DeleteArguments();

    CFastHashMap<const void*, CValues*> knownTables;

    // Start reading arguments until there are none left
    while (lua_type(luaVM, uiIndexBegin) != LUA_TNONE)
    {
        // Create an argument, let it read out the argument and add it to our vector
        CValue* pArgument = new CValue(luaVM, uiIndexBegin++, &knownTables);
        m_Arguments.push_back(pArgument);

        knownTables.clear();
    }
}

void CValues::ReadTable(lua_State* luaVM, int iIndexBegin, CFastHashMap<const void*, CValues*>* pKnownTables)
{
    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new CFastHashMap<const void*, CValues*>();
        bKnownTablesCreated = true;
    }

    pKnownTables->insert(std::make_pair(lua_topointer(luaVM, iIndexBegin), this));

    // Delete the previous arguments if any
    DeleteArguments();

    LUA_CHECKSTACK(luaVM, 2);
    lua_pushnil(luaVM); /* first key */
    if (iIndexBegin < 0)
        iIndexBegin--;

    while (lua_next(luaVM, iIndexBegin) != 0)
    {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        CValue* pArgument = new CValue(luaVM, -2, pKnownTables);
        m_Arguments.push_back(pArgument);            // push the key first

        pArgument = new CValue(luaVM, -1, pKnownTables);
        m_Arguments.push_back(pArgument);            // then the value

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(luaVM, 1);
    }

    if (bKnownTablesCreated)
        delete pKnownTables;
}

void CValues::Read(lua_State* luaVM, int iIndex)
{
    CValue* pArgument = new CValue(luaVM, iIndex);
    m_Arguments.push_back(pArgument);
}

void CValues::Write(lua_State* luaVM) const
{
    // Push all our arguments
    vector<CValue*>::const_iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end(); iter++)
    {
        (*iter)->Push(luaVM);
    }
}

void CValues::PushAsTable(lua_State* luaVM, CFastHashMap<CValues*, int>* pKnownTables) const
{
    // Ensure there is enough space on the Lua stack
    LUA_CHECKSTACK(luaVM, 4);

    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new CFastHashMap<CValues*, int>();
        bKnownTablesCreated = true;

        lua_newtable(luaVM);
        // using registry to make it fail safe, else we'd have to carry
        // either lua top or current depth variable between calls
        lua_setfield(luaVM, LUA_REGISTRYINDEX, "cache");
    }

    lua_newtable(luaVM);

    // push it onto the known tables
    int size = pKnownTables->size();
    lua_getfield(luaVM, LUA_REGISTRYINDEX, "cache");
    lua_pushnumber(luaVM, ++size);
    lua_pushvalue(luaVM, -3);
    lua_settable(luaVM, -3);
    lua_pop(luaVM, 1);
    pKnownTables->insert(std::make_pair((CValues*)this, size));

    vector<CValue*>::const_iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end() && (iter + 1) != m_Arguments.end(); iter++)
    {
        (*iter)->Push(luaVM, pKnownTables);            // index
        iter++;
        (*iter)->Push(luaVM, pKnownTables);            // value
        lua_settable(luaVM, -3);
    }

    if (bKnownTablesCreated)
    {
        // clear the cache
        lua_pushnil(luaVM);
        lua_setfield(luaVM, LUA_REGISTRYINDEX, "cache");
        delete pKnownTables;
    }
}

void CValues::Write(const CValues& Arguments)
{
    vector<CValue*>::const_iterator iter = Arguments.IterBegin();
    for (; iter != Arguments.IterEnd(); iter++)
    {
        CValue* pArgument = new CValue(**iter);
        m_Arguments.push_back(pArgument);
    }
}

bool CValues::Call(CLuaMain* pLuaMain, const CLuaFunctionRef& iLuaFunction, CValues* returnValues) const
{
    assert(pLuaMain);
    TIMEUS startTime = GetTimeUs();

    // Add the function name to the stack and get the event from the table
    lua_State* luaVM = pLuaMain->GetVirtualMachine();
    assert(luaVM);
    LUA_CHECKSTACK(luaVM, 2);
    int luaStackPointer = lua_gettop(luaVM);
    lua_getref(luaVM, iLuaFunction.ToInt());

    // Push our arguments onto the stack
    Write(luaVM);

    // Call the function with our arguments
    pLuaMain->ResetInstructionCount();

    int iret = pLuaMain->PCall(luaVM, m_Arguments.size(), LUA_MULTRET, 0);
    if (iret == LUA_ERRRUN || iret == LUA_ERRMEM)
    {
        SString strRes = ConformResourcePath(lua_tostring(luaVM, -1));
        g_pClientGame->GetScriptDebugging()->LogPCallError(luaVM, strRes);

        // cleanup the stack
        while (lua_gettop(luaVM) - luaStackPointer > 0)
            lua_pop(luaVM, 1);

        return false;            // the function call failed
    }
    else
    {
        int iReturns = lua_gettop(luaVM) - luaStackPointer;

        if (returnValues != NULL)
        {
            for (int i = -iReturns; i <= -1; i++)
            {
                returnValues->Read(luaVM, i);
            }
        }

        // cleanup the stack
        while (lua_gettop(luaVM) - luaStackPointer > 0)
            lua_pop(luaVM, 1);
    }

    CClientPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(pLuaMain, pLuaMain->GetFunctionTag(iLuaFunction.ToInt()), GetTimeUs() - startTime);
    return true;
}

bool CValues::CallGlobal(CLuaMain* pLuaMain, const char* szFunction, CValues* returnValues) const
{
    assert(pLuaMain);
    assert(szFunction);
    TIMEUS startTime = GetTimeUs();

    // Add the function name to the stack and get the event from the table
    lua_State* luaVM = pLuaMain->GetVirtualMachine();
    assert(luaVM);
    LUA_CHECKSTACK(luaVM, 1);
    int luaStackPointer = lua_gettop(luaVM);
    lua_pushstring(luaVM, szFunction);
    lua_gettable(luaVM, LUA_GLOBALSINDEX);

    // If that function doesn't exist, return false
    if (lua_isnil(luaVM, -1))
    {
        // cleanup the stack
        while (lua_gettop(luaVM) - luaStackPointer > 0)
            lua_pop(luaVM, 1);

        return false;
    }

    // Push our arguments onto the stack
    Write(luaVM);

    // Reset function call timer (checks long-running functions)
    pLuaMain->ResetInstructionCount();

    // Call the function with our arguments
    int iret = pLuaMain->PCall(luaVM, m_Arguments.size(), LUA_MULTRET, 0);
    if (iret == LUA_ERRRUN || iret == LUA_ERRMEM)
    {
        std::string strRes = ConformResourcePath(lua_tostring(luaVM, -1));
        g_pClientGame->GetScriptDebugging()->LogPCallError(luaVM, strRes);

        // cleanup the stack
        while (lua_gettop(luaVM) - luaStackPointer > 0)
            lua_pop(luaVM, 1);

        return false;            // the function call failed
    }
    else
    {
        int iReturns = lua_gettop(luaVM) - luaStackPointer;

        if (returnValues != NULL)
        {
            for (int i = -iReturns; i <= -1; i++)
            {
                returnValues->Read(luaVM, i);
            }
        }

        // cleanup the stack
        while (lua_gettop(luaVM) - luaStackPointer > 0)
            lua_pop(luaVM, 1);
    }

    CClientPerfStatLuaTiming::GetSingleton()->UpdateLuaTiming(pLuaMain, szFunction, GetTimeUs() - startTime);
    return true;
}

CValue* CValues::PushNil()
{
    CValue* pArgument = new CValue;
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(bool bBool)
{
    CValue* pArgument = new CValue();
    pArgument->ReadBool(bBool);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(double dNumber)
{
    CValue* pArgument = new CValue();
    pArgument->ReadNumber(dNumber);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(const std::string& strString)
{
    CValue* pArgument = new CValue();
    pArgument->ReadString(strString);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(CResource* pResource)
{
    CValue* pArgument = new CValue;
    pArgument->ReadScriptID(pResource->GetScriptID());
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(CClientEntity* pElement)
{
    CValue* pArgument = new CValue;
    pArgument->ReadElement(pElement);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(const CValue& Argument)
{
    CValue* pArgument = new CValue(Argument);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

CValue* CValues::Push(CValues* table)
{
    CValue* pArgument = new CValue();
    pArgument->ReadTable(table);
    m_Arguments.push_back(pArgument);
    return pArgument;
}

// Gets rid of all the arguments in the list
void CValues::DeleteArguments()
{
    // Delete each item
    vector<CValue*>::iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end(); iter++)
    {
        delete *iter;
    }

    // Clear the vector
    m_Arguments.clear();
}

// Gets rid of the last argument in the list
void CValues::Pop()
{
    // Delete the last element
    CValue* item = m_Arguments.back();
    delete item;

    // Pop it out of the vector
    m_Arguments.pop_back();
}

void CValues::ValidateTableKeys()
{
    // Iterate over m_Arguments as pairs
    // If first is LUA_TNIL, then remove pair
    vector<CValue*>::iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end();)
    {
        // Check first in pair
        if ((*iter)->GetType() == LUA_TNIL)
        {
            // TODO - Handle ref in KnownTables
            // Remove pair
            delete *iter;
            iter = m_Arguments.erase(iter);
            if (iter != m_Arguments.end())
            {
                delete *iter;
                iter = m_Arguments.erase(iter);
            }
            // Check if end
            if (iter == m_Arguments.end())
                break;
        }
        else
        {
            // Skip second in pair
            iter++;
            // Check if end
            if (iter == m_Arguments.end())
                break;

            iter++;
        }
    }
}

bool CValues::Read(NetBitStreamInterface& bitStream, std::vector<CValues*>* pKnownTables)
{
    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new std::vector<CValues*>();
        bKnownTablesCreated = true;
    }

    unsigned int uiNumArgs;
    if (bitStream.ReadCompressed(uiNumArgs))
    {
        pKnownTables->push_back(this);
        for (unsigned int ui = 0; ui < uiNumArgs; ++ui)
        {
            CValue* pArgument = new CValue(bitStream, pKnownTables);
            m_Arguments.push_back(pArgument);
        }
    }

    if (bKnownTablesCreated)
        delete pKnownTables;

    return true;
}

bool CValues::Write(NetBitStreamInterface& bitStream, CFastHashMap<CValues*, unsigned long>* pKnownTables) const
{
    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new CFastHashMap<CValues*, unsigned long>();
        bKnownTablesCreated = true;
    }

    bool bSuccess = true;
    pKnownTables->insert(make_pair((CValues*)this, pKnownTables->size()));
    bitStream.WriteCompressed(static_cast<unsigned int>(m_Arguments.size()));

    vector<CValue*>::const_iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end(); iter++)
    {
        CValue* pArgument = *iter;
        if (!pArgument->Write(bitStream, pKnownTables))
        {
            bSuccess = false;
        }
    }

    if (bKnownTablesCreated)
        delete pKnownTables;

    return bSuccess;
}

bool CValues::Write(std::string& strJSON, bool bSerialize, int flags)
{
    json_object* my_array = WriteToJSONArray(bSerialize);
    if (my_array)
    {
        strJSON = json_object_to_json_string_ext(my_array, flags);
        json_object_put(my_array);            // dereference - causes a crash, is actually commented out in the example too
        return true;
    }
    return false;
}

json_object* CValues::WriteToJSONArray(bool bSerialize)
{
    json_object*                          my_array = json_object_new_array();
    vector<CValue*>::const_iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end(); iter++)
    {
        CValue* pArgument = *iter;
        json_object*  object = pArgument->WriteToJSONObject(bSerialize);
        if (object)
        {
            json_object_array_add(my_array, object);
        }
        else
        {
            break;
        }
    }
    return my_array;
}

json_object* CValues::WriteTableToJSONObject(bool bSerialize, CFastHashMap<CValues*, unsigned long>* pKnownTables)
{
    bool bKnownTablesCreated = false;
    if (!pKnownTables)
    {
        pKnownTables = new CFastHashMap<CValues*, unsigned long>();
        bKnownTablesCreated = true;
    }

    pKnownTables->insert(std::make_pair(this, pKnownTables->size()));

    bool                                  bIsArray = true;
    unsigned int                          iArrayPos = 1;            // lua arrays are 1 based
    vector<CValue*>::const_iterator iter = m_Arguments.begin();
    for (; iter != m_Arguments.end(); iter += 2)
    {
        CValue* pArgument = *iter;
        if (pArgument->GetType() == LUA_TNUMBER)
        {
            double       num = pArgument->GetNumber();
            unsigned int iNum = static_cast<unsigned int>(num);
            if (num == iNum)
            {
                if (iArrayPos != iNum)            // check if the value matches its index in the table
                {
                    bIsArray = false;
                    break;
                }
            }
            else
            {
                bIsArray = false;
                break;
            }
        }
        else
        {
            bIsArray = false;
            break;
        }
        iArrayPos++;
    }

    if (bIsArray)
    {
        json_object*                          my_array = json_object_new_array();
        vector<CValue*>::const_iterator iter = m_Arguments.begin();
        for (; iter != m_Arguments.end(); iter++)
        {
            iter++;            // skip the key values
            CValue* pArgument = *iter;
            json_object*  object = pArgument->WriteToJSONObject(bSerialize, pKnownTables);
            if (object)
            {
                json_object_array_add(my_array, object);
            }
            else
            {
                break;
            }
        }
        if (bKnownTablesCreated)
            delete pKnownTables;
        return my_array;
    }
    else
    {
        json_object* my_object = json_object_new_object();
        iter = m_Arguments.begin();
        for (; iter != m_Arguments.end(); iter++)
        {
            char szKey[255];
            szKey[0] = '\0';
            CValue* pArgument = *iter;
            if (!pArgument->WriteToString(szKey, 255))            // index
                break;
            iter++;
            pArgument = *iter;
            json_object* object = pArgument->WriteToJSONObject(bSerialize, pKnownTables);            // value

            if (object)
            {
                json_object_object_add(my_object, szKey, object);
            }
            else
            {
                break;
            }
        }
        if (bKnownTablesCreated)
            delete pKnownTables;
        return my_object;
    }
}

bool CValues::Read(const char* szJSON)
{
    // Fast isJSON check: Check first non-white space character is '[' or '{'
    for (const char* ptr = szJSON; true;)
    {
        char c = *ptr++;
        if (c == '[' || c == '{')
            break;
        if (isspace((uchar)c))
            continue;
        return false;
    }

    json_object* object = json_tokener_parse(szJSON);
    if (object)
    {
        if (json_object_get_type(object) == json_type_array)
        {
            bool bSuccess = true;

            std::vector<CValues*> knownTables;

            for (uint i = 0; i < json_object_array_length(object); i++)
            {
                json_object*  arrayObject = json_object_array_get_idx(object, i);
                CValue* pArgument = new CValue();
                bSuccess = pArgument->ReadFromJSONObject(arrayObject, &knownTables);
                m_Arguments.push_back(pArgument);            // then the value
                if (!bSuccess)
                    break;
            }
            json_object_put(object);            // dereference
            return bSuccess;
        }
        else if (json_object_get_type(object) == json_type_object)
        {
            std::vector<CValues*> knownTables;
            CValue*               pArgument = new CValue();
            bool                        bSuccess = pArgument->ReadFromJSONObject(object, &knownTables);
            m_Arguments.push_back(pArgument);            // value
            json_object_put(object);

            return bSuccess;
        }
        json_object_put(object);            // dereference
    }
    //    else
    //        g_pClientGame->GetScriptDebugging()->LogError ( "Could not parse invalid JSON object.");
    //   else
    //        g_pClientGame->GetScriptDebugging()->LogError ( "Could not parse HTTP POST request, ensure data uses JSON.");
    return false;
}

bool CValues::ReadFromJSONObject(json_object* object, std::vector<CValues*>* pKnownTables)
{
    if (object)
    {
        if (json_object_get_type(object) == json_type_object)
        {
            bool bKnownTablesCreated = false;
            if (!pKnownTables)
            {
                pKnownTables = new std::vector<CValues*>();
                bKnownTablesCreated = true;
            }

            pKnownTables->push_back(this);

            bool bSuccess = true;
            json_object_object_foreach(object, key, val)
            {
                CValue* pArgument = new CValue();
                pArgument->ReadString(key);
                m_Arguments.push_back(pArgument);            // push the key first
                pArgument = new CValue();
                bSuccess = pArgument->ReadFromJSONObject(val, pKnownTables);            // then the value
                m_Arguments.push_back(pArgument);
                if (!bSuccess)
                    break;
            }

            if (bKnownTablesCreated)
                delete pKnownTables;
            return bSuccess;
        }
    }
    //   else
    //        g_pClientGame->GetScriptDebugging()->LogError ( "Could not parse invalid JSON object.");
    return false;
}

bool CValues::ReadFromJSONArray(json_object* object, std::vector<CValues*>* pKnownTables)
{
    if (object)
    {
        if (json_object_get_type(object) == json_type_array)
        {
            bool bKnownTablesCreated = false;
            if (!pKnownTables)
            {
                pKnownTables = new std::vector<CValues*>();
                bKnownTablesCreated = true;
            }

            pKnownTables->push_back(this);

            bool bSuccess = true;
            for (uint i = 0; i < json_object_array_length(object); i++)
            {
                json_object*  arrayObject = json_object_array_get_idx(object, i);
                CValue* pArgument = new CValue();
                pArgument->ReadNumber(i + 1);            // push the key
                m_Arguments.push_back(pArgument);

                pArgument = new CValue();
                bSuccess = pArgument->ReadFromJSONObject(arrayObject, pKnownTables);
                m_Arguments.push_back(pArgument);            // then the valoue
                if (!bSuccess)
                    break;
            }

            if (bKnownTablesCreated)
                delete pKnownTables;
            return bSuccess;
        }
    }
    //    else
    //        g_pClientGame->GetScriptDebugging()->LogError ( "Could not parse invalid JSON object.");
    return false;
}
