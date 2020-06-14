/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/luadefs/CLuaPrimitiveBufferDefs.cpp
 *  PURPOSE:     Lua definitions class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include <array>

void CLuaPrimitiveBufferDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"dxCreatePrimitiveBuffer", DxCreatePrimitiveBuffer},

        {"dxCreatePrimitiveBufferAddVertices", DxCreatePrimitiveBufferAddVertices},
        {"dxCreatePrimitiveBufferAddIndices", DxCreatePrimitiveBufferAddIndices},

        {"dxCreatePrimitiveBufferSetVertices", DxCreatePrimitiveBufferSetVertices},
        {"dxCreatePrimitiveBufferSetIndices", DxCreatePrimitiveBufferSetIndices},

        {"dxCreatePrimitiveBufferAddVertices", DxCreatePrimitiveBufferAddVertices},
        {"dxCreatePrimitiveBufferIndices", DxCreatePrimitiveBufferAddIndices},

        //{"dxCreatePrimitiveBufferClearVertices", DxCreatePrimitiveBufferClearVertices},
        //{"dxCreatePrimitiveBufferClearIndices", DxCreatePrimitiveBufferClearIndices},

        {"dxDrawPrimitiveBuffer", DxDrawPrimitiveBuffer},
        {"dxDrawPrimitiveBuffer3D", DxDrawPrimitiveBuffer3D},
    };

    // Add functions
    for (const auto& [name, function] : functions)
        CLuaCFunctions::AddFunction(name, function);
}

void CLuaPrimitiveBufferDefs::AddClass(lua_State* luaVM)
{
    lua_newclass(luaVM);

    constexpr static const std::pair<const char*, const char*> classFunctions[]{
        {"create", "dxCreatePrimitiveBuffer"},

        {"addVertices", "dxCreatePrimitiveBufferAddVertices"},
        {"addIndices", "dxCreatePrimitiveBufferAddIndices"},

        {"setVertices", "dxCreatePrimitiveBufferAddVertices"},
        {"setIndices", "dxCreatePrimitiveBufferAddIndices"},

        {"draw", "dxDrawPrimitiveBuffer"},
        {"draw3D", "dxDrawPrimitiveBuffer3D"}
    };

    for (const auto& [methodName, functioName] : classFunctions)
        lua_classfunction(luaVM, methodName, functioName);

    lua_registerclass(luaVM, "DxPrimitiveBuffer", "Element");
}

int CLuaPrimitiveBufferDefs::DxDrawPrimitiveBuffer(lua_State* luaVM)
{
    // bool dxDrawPrimitiveBuffer( primitive-buffer buffer, vector3 position, vector3 rotation, vector3 scale, bool postGui, primitive-view primitiveView, mixed
    // material)
    CClientPrimitiveBuffer* pPrimitiveBuffer;
    CVector                 vecPosition;
    CVector                 vecRotation;
    CVector                 vecScale;
    bool                    bPostGui;
    ePrimitiveView          primitiveView;
    CClientMaterial*        pMaterialElement = nullptr;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(pPrimitiveBuffer);
    if (!argStream.HasErrors())
    {
        if (pPrimitiveBuffer->IsRequireMaterial())
            MixedReadMaterialString(argStream, pMaterialElement);
        argStream.ReadVector3D(vecPosition);
        argStream.ReadVector3D(vecRotation, CVector(0, 0, 0));
        argStream.ReadVector3D(vecScale, CVector(1, 1, 1));
        argStream.ReadBool(bPostGui, false);
        argStream.ReadEnumString(primitiveView, (ePrimitiveView)0);

        if (!argStream.HasErrors())
        {
            ConvertDegreesToRadians(vecRotation);

            PrimitiveBufferSettings bufferSettings;
            bufferSettings.matrix.SetPosition(vecPosition);
            bufferSettings.matrix.SetRotation(vecRotation);
            bufferSettings.matrix.SetScale(vecScale);
            bufferSettings.eView = primitiveView;
            if (pMaterialElement)
                bufferSettings.pMaterial = pMaterialElement->GetMaterialItem();
            else
                bufferSettings.pMaterial = nullptr;

            g_pCore->GetGraphics()->DrawPrimitiveBufferQueued(reinterpret_cast<CClientPrimitiveBufferInterface*>(pPrimitiveBuffer), bufferSettings, bPostGui);

            lua_pushboolean(luaVM, true);
            return 1;
        }
        if (argStream.HasErrors())
            m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }
    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaPrimitiveBufferDefs::DxDrawPrimitiveBuffer3D(lua_State* luaVM)
{
    // bool dxDrawPrimitiveBuffer3D( primitive-buffer buffer, vector3 position, vector3 rotation, vector3 scale, bool postGui, primitive-view primitiveView, mixed
    // material)

    CClientPrimitiveBuffer* pPrimitiveBuffer;
    CVector                 vecPosition;
    CVector                 vecRotation;
    CVector                 vecScale;
    bool                    bPostGui;
    ePrimitiveView          primitiveView;
    CClientMaterial*        pMaterialElement = nullptr;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(pPrimitiveBuffer);
    if (!argStream.HasErrors())
    {
        if (pPrimitiveBuffer->IsRequireMaterial())
            MixedReadMaterialString(argStream, pMaterialElement);
        argStream.ReadVector3D(vecPosition);
        argStream.ReadVector3D(vecRotation, CVector(0, 0, 0));
        argStream.ReadVector3D(vecScale, CVector(1, 1, 1));
        argStream.ReadBool(bPostGui, false);
        argStream.ReadEnumString(primitiveView, (ePrimitiveView)0);

        if (!argStream.HasErrors())
        {
            ConvertDegreesToRadians(vecRotation);

            PrimitiveBufferSettings bufferSettings;
            bufferSettings.matrix.SetPosition(vecPosition);
            bufferSettings.matrix.SetRotation(vecRotation);
            bufferSettings.matrix.SetScale(vecScale);
            bufferSettings.eView = primitiveView;
            if (pMaterialElement)
                bufferSettings.pMaterial = pMaterialElement->GetMaterialItem();
            else
                bufferSettings.pMaterial = nullptr;

            g_pCore->GetGraphics()->DrawPrimitiveBuffer3DQueued(reinterpret_cast<CClientPrimitiveBufferInterface*>(pPrimitiveBuffer), bufferSettings, bPostGui);

            lua_pushboolean(luaVM, true);
            return 1;
        }
    }
    if (argStream.HasErrors())
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());

    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaPrimitiveBufferDefs::DxCreatePrimitiveBufferAddVertices(lua_State* luaVM)
{
    /* TODO */
}

int CLuaPrimitiveBufferDefs::DxCreatePrimitiveBufferAddIndices(lua_State* luaVM)
{
    /* TODO */
}

int CLuaPrimitiveBufferDefs::DxCreatePrimitiveBufferSetVertices(lua_State* luaVM)
{
    CClientPrimitiveBuffer* primitiveBuffer;
    int                     primitiveFlags;
    D3DPRIMITIVETYPE        primitiveType;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(primitiveBuffer);

    // Read flags from string and create a flag from it
    int primitiveFlags = 0;
    {
        std::vector<ePrimitiveFlag> flags;
        argStream.ReadEnumStringList(flags, 0); // Read primitive formats as a comma separated list

        for (ePrimitiveFlag flag : flags)
            primitiveFlags |= flag;
    }

    argStream.ReadEnumString(primitiveType);

    if (!argStream.NextIsTable())
    {
        argStream.SetTypeError("table");
        goto log_error_push_false_return;
    }

    const bool isXYZ = primitiveFlags & ePrimitiveFlag::XYZ;
    const bool isXY = primitiveFlags & ePrimitiveFlag::XY;
    if (isXYZ == isXY) // Only 1 needs to be true at a time
    {
        m_pScriptDebugging->LogCustom(luaVM, "Primitive settings flag must contain either `xy` or `xyz` declaration, and not both");
        goto push_false_return;
    }

    // Calculate expected Lua vertex table size
    size_t expectedLuaVertexTableSize = 0;
    for (auto [primitiveDataType, size] : s_flagToTableSizeMap)
        if (primitiveFlags & primitiveDataType) // Check if flag was specified
            expectedLuaVertexTableSize += size;

    std::vector<CVector>   vertices;
    std::vector<CVector2D> uvs;
    std::vector<D3DCOLOR>  diffuses;

    const bool usesUVs     = primitiveFlags & ePrimitiveFlag::UV;
    const bool usesDiffuse = primitiveFlags & ePrimitiveFlag::DIFFUSE;

    //
    // TODO: Make this unpack the table on the stack(if thats possible)
    // And read coords and thing with the appropriate functions(readVector, readColor, etc..)
    //
    size_t currentVertexIndex = 0;
    while (argStream.NextIsTable() && !argStream.HasErrors()) 
    {
        size_t tableReadIndex = 0;

        float values[6]; // We need to read 6 values at most(no problem if there's less, since we pass in luaVertexTableSize)
        size_t readCount = argStream.ReadTable(values, expectedLuaVertexTableSize); // Read whatever amount needed 

        if (readCount != expectedLuaVertexTableSize)
        {
            m_pScriptDebugging->LogCustom(luaVM, 
                SString("Invalid primitive vertex at index %u. Expected amount of numbers: %u [Got %u]", currentVertexIndex, expectedLuaVertexTableSize, readCount).c_str());
            goto push_false_return;
        }
        currentVertexIndex++;

        vertices.emplace_back(values[tableReadIndex++], values[tableReadIndex++], isXYZ ? values[tableReadIndex++] : 0);

        if (usesDiffuse)
            diffuses.emplace_back(static_cast<unsigned long>(static_cast<int64_t>(values[tableReadIndex++])));

        if (usesUVs)
            uvs.emplace_back(values[tableReadIndex++], values[tableReadIndex++]);
    }

    if (!g_pCore->GetGraphics()->IsValidPrimitiveSize(vertices.size(), primitiveType))
    {
        m_pScriptDebugging->LogCustom(luaVM, SString("Vertices amount %i is invalid for selected vertices type", vertices.size()).c_str());
        goto push_false_return;
    }

    int flagFVF = D3DFVF_XYZ;

    primitiveBuffer->SetPrimitiveType(primitiveType);
    primitiveBuffer->AddVertexBuffer(vertices, ePrimitiveFlag::XYZ);

    if (usesUVs)
    {
        flagFVF |= D3DFVF_TEX1;
        primitiveBuffer->SetRequireMaterial(true);
        primitiveBuffer->AddVertexBuffer(uvs, ePrimitiveFlag::UV);
    }

    if (usesDiffuse)
    {
        flagFVF |= D3DFVF_DIFFUSE;
        primitiveBuffer->AddVertexBuffer(diffuses, ePrimitiveFlag::DIFFUSE);
    }

    primitiveBuffer->SetFVF(flagFVF);
    primitiveBuffer->Finalize();


    if (argStream.HasErrors())
    {
log_error_push_false_return:
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }

push_false_return:
    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaPrimitiveBufferDefs::DxCreatePrimitiveBufferSetIndices(lua_State* luaVM)
{
    // TODO: Add support for passing in table as first argument instead of numbers
    // Because unpack() cant unpack more than ~8000 table values
    // And using addIndices is slow

    CClientPrimitiveBuffer* primitiveBuffer;
    bool                    isZeroBased;
    std::vector<uint32_t>   indices;

    CScriptArgReader argStream(luaVM);
    argStream.ReadUserData(primitiveBuffer);
    argStream.ReadIfNextIsBool(isZeroBased, false);

    if (!argStream.HasErrors())
    {
        // lua_gettop returns the number of arguments passed to the function
        indices.reserve(lua_gettop(luaVM));

        // We automatically figure out indices key type(16b or 32b), no need for the user to do so
        // Because we'd need to recheck for overflow anyways
        bool use32BitIndices = false;
        while (argStream.NextIsNumber())
        {
            uint32_t& emplaced = indices.emplace_back(); // Emplace
            argStream.ReadNumber(emplaced); // Read value into it
            
            use32BitIndices |= emplaced > std::numeric_limits<uint16_t>::max();
        }

        if (!indices.empty() && indices.size() % 3 == 0)
        {
            if (!isZeroBased)
            {
                // Convert from 1 based indexing to 0 based if needed
                // Error if i == 0 before substracting
                for (auto& i : indices)
                {
                    if (i == 0)
                    {
                        m_pScriptDebugging->LogCustom(luaVM, "Indices start at 1, not 0. Tip: set bIsZeroBased to true to enable 0-based indexing");
                        goto push_false_and_return;
                    }
                    i--;
                }
            }

            primitiveBuffer->CreateIndexBuffer(indices, !use32BitIndices);
            lua_pushboolean(luaVM, true);
            return 1;
        }
        else
        {
            m_pScriptDebugging->LogCustom(luaVM, "The number of indices must be divisible by 3, and greater than 0.");
            goto push_false_and_return;
        }
    }
    else
    {
log_error_push_false_return:
        m_pScriptDebugging->LogCustom(luaVM, argStream.GetFullErrorMessage());
    }

push_false_and_return:
    lua_pushboolean(luaVM, false);
    return 1;
}

int CLuaPrimitiveBufferDefs::DxCreatePrimitiveBuffer(lua_State* luaVM)
{
    // primitive-buffer dxCreatePrimitiveBuffer( string primitiveDataTypeFlags, primitiveType primitiveType)

    D3DPRIMITIVETYPE            primitiveType;

    /*
    IMPLEMENT_ENUM_BEGIN(ePrimitiveFlag)
    ADD_ENUM(ePrimitiveFlag::XYZ, "xyz")
    ADD_ENUM(ePrimitiveFlag::XY, "xy")
    ADD_ENUM(ePrimitiveFlag::UV, "uv")
    ADD_ENUM(ePrimitiveFlag::DIFFUSE, "diffuse")
    ADD_ENUM(ePrimitiveFlag::INDICES16, "16bitindices")
    ADD_ENUM(ePrimitiveFlag::INDICES32, "32bitindices")
    IMPLEMENT_ENUM_END("primitive-format")

    */

    /*
    enum ePrimitiveFlag
{
    ePrimitiveFlag::XYZ = 1 << 0,
    ePrimitiveFlag::XY = 1 << 1,
    ePrimitiveFlag::UV = 1 << 2,
    ePrimitiveFlag::DIFFUSE = 1 << 3,
    ePrimitiveFlag::INDICES16 = 1 << 29,
    ePrimitiveFlag::INDICES32 = 1 << 30,
};
*/

    CScriptArgReader argStream(luaVM);

    // Read flags and create a flag from it
    int primitiveDataTypesFlag = 0;
    {
        std::vector<ePrimitiveFlag> flags;
        argStream.ReadEnumStringList(flags, 0); // Read primitive formats as a comma separated list

        for (ePrimitiveFlag flag : flags)
            primitiveDataTypesFlag |= flag;
    }

    argStream.ReadEnumString(primitiveType);

    if (!primitiveDataTypesFlag)
    {
        m_pScriptDebugging->LogCustom(luaVM, "Primitive data declaration is empty or invalid.");
        lua_pushboolean(luaVM, false);
        return 1;
    }


    // Check if flag contains both xy and xyz
    {
        bool isXY = primitiveDataTypesFlag & ePrimitiveFlag::XY;
        bool isXYZ = primitiveDataTypesFlag & ePrimitiveFlag::XYZ;
        if (isXY || isXYZ)
        {
            if (isXY && isXYZ)
            {
                m_pScriptDebugging->LogCustom(luaVM, "Using 'xy' and 'xyz' at the same type in primitveFormats is forbidden");
                lua_pushboolean(luaVM, false);
                return 1;
            }
        }
        else
        {
            m_pScriptDebugging->LogCustom(luaVM, "Argument primitiveFormats must specify either 'xyz' or 'xy");
            lua_pushboolean(luaVM, false);
            return 1;
        }
    }



    std::vector<uint32_t> indexList;
    if (argStream.NextIsTable())
    {
        bool is16Bit = primitiveDataTypesFlag & ePrimitiveFlag::INDICES16;
        bool is32Bit = primitiveDataTypesFlag & ePrimitiveFlag::INDICES32;

        // Check if flag contains both 16 and 32 bit flags
        if (is32Bit && is16Bit)
        {
            m_pScriptDebugging->LogCustom(luaVM, "You can not use `16bitindices` and `32bitindices` data declaration at once.");
            lua_pushboolean(luaVM, false);
            return 1;
        } 

        argStream.ReadNumberTable(indexList);
        
        if (indexList.size() > 0 && indexList.size() % 3 == 0)
        {

        }
    }

    std::vector<CVector>   vecXYZ;
    std::vector<CVector2D> vecUV;
    std::vector<D3DCOLOR>  vecDiffuse;


    int vertexCount = 0;

    // Calculate lua table size
    for (auto [primitiveDataType, size] : s_flagToTableSizeMap)
        if (primitiveDataTypesFlag & (int)primitiveDataType) // Check if type was specified
            vertexCount += s_flagToTableSizeMap[primitiveDataType];

    int tableOffset = 0;
    int vertexIndex = 0;

    while (argStream.NextIsTable())
    {
        CLuaArguments arguments;
        argStream.ReadLuaArguments(arguments);

        vertexIndex++;
        tableOffset = 0;
        tableContent.clear();
        argStream.ReadNumberTable(tableContent);
        if (tableContent.size() != vertexCount)
        {
            m_pScriptDebugging->LogCustom(
                luaVM,
                SString("Primitive vertex require %i number, got %i numbers, vertex index %i.", vertexCount, tableContent.size(), vertexIndex).c_str());
            lua_pushboolean(luaVM, false);
            return 1;
        }

        if ((primitiveDataTypesFlag & ePrimitiveFlag::XY) == ePrimitiveFlag::XY)
        {
            vecXYZ.emplace_back(tableContent[tableOffset], tableContent[tableOffset + 1], 0);
            tableOffset += s_flagToTableSizeMap[ePrimitiveFlag::XY];
        }

        if (primitiveDataTypesFlag & ePrimitiveFlag::XYZ)
        {
            vecXYZ.emplace_back(tableContent[tableOffset], tableContent[tableOffset + 1], tableContent[tableOffset + 2]);
            tableOffset += s_flagToTableSizeMap[ePrimitiveFlag::XYZ];
        }

        if ((primitiveDataTypesFlag & ePrimitiveFlag::DIFFUSE) == ePrimitiveFlag::DIFFUSE)
        {
            vecDiffuse.emplace_back(static_cast<unsigned long>(static_cast<int64_t>(tableContent[tableOffset])));
            tableOffset += s_flagToTableSizeMap[ePrimitiveFlag::DIFFUSE];
        }

        if ((primitiveDataTypesFlag & ePrimitiveFlag::UV) == ePrimitiveFlag::UV)
        {
            vecUV.emplace_back(tableContent[tableOffset], tableContent[tableOffset + 1]);
            tableOffset += s_flagToTableSizeMap[ePrimitiveFlag::UV];
        }
    }

    if (indexList32Bit.size() > 0)
    {
        int i = 0;
        for (auto& index : indexList32Bit)
        {
            i++;
            if (index > vecXYZ.size())
            {
                m_pScriptDebugging->LogCustom(luaVM, SString("Indices table contains index %i out of range at %i table index.", index, i).c_str());
                lua_pushboolean(luaVM, false);
                return 1;
            }
        }
    }
    if (indexList16Bit.size() > 0)
    {
        int i = 0;
        for (auto& index : indexList16Bit)
        {
            i++;
            if (index > vecXYZ.size())
            {
                m_pScriptDebugging->LogCustom(luaVM, SString("Indices table contains index %i out of range at %i table index.", index, i).c_str());
                lua_pushboolean(luaVM, false);
                return 1;
            }
        }
    }

    if (contain16BitIndices || contain32BitIndices)
    {
        if (vecXYZ.size() < 1)
        {
            m_pScriptDebugging->LogCustom(luaVM, SString("You must add at least 1 vertex while using indices.", vecXYZ.size()).c_str());
            lua_pushboolean(luaVM, false);
            return 1;
        }
    }
    else if (!g_pCore->GetGraphics()->IsValidPrimitiveSize(vecXYZ.size(), primitiveType))
    {
        m_pScriptDebugging->LogCustom(luaVM, SString("Vertices amount %i is invalid selected vertices type", vecXYZ.size()).c_str());
        lua_pushboolean(luaVM, false);
        return 1;
    }

    if (!argStream.HasErrors())
    {
        CLuaMain* luaMain = m_pLuaManager->GetVirtualMachine(luaVM);
        if (luaMain)
        {
            CResource* pResource = luaMain->GetResource();
            if (pResource)
            {
                // Create it and return it
                CClientPrimitiveBuffer* primitiveBuffer = CStaticFunctionDefinitions::CreatePrimitiveBuffer(*pResource);
                if (primitiveBuffer)
                {
                    CElementGroup* elementGroup = pResource->GetElementGroup();
                    if (elementGroup)
                    {
                        elementGroup->Add(primitiveBuffer);
                    }

                    primitiveBuffer->SetPrimitiveType(primitiveType);

                    int FVF = D3DFVF_XYZ;
                    primitiveBuffer->AddVertexBuffer(vecXYZ, ePrimitiveFlag::XYZ);

                    if (vecUV.size() > 0)
                    {
                        FVF |= D3DFVF_TEX1;
                        primitiveBuffer->SetRequireMaterial(true);
                        primitiveBuffer->AddVertexBuffer(vecUV, ePrimitiveFlag::UV);
                    }
                    if (vecDiffuse.size() > 0)
                    {
                        FVF |= D3DFVF_DIFFUSE;
                        primitiveBuffer->AddVertexBuffer(vecDiffuse, ePrimitiveFlag::DIFFUSE);
                    }

                    if (indexList16Bit.size() > 0)
                    {
                        primitiveBuffer->CreateIndexBuffer(indexList16Bit);
                    }
                    else if (indexList32Bit.size() > 0)
                    {
                        primitiveBuffer->CreateIndexBuffer(indexList32Bit);
                    }

                    primitiveBuffer->SetFVF(FVF);
                    primitiveBuffer->Finalize();

                    lua_pushelement(luaVM, primitiveBuffer);
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
