#include "StdInc.h"

class CLuaUserData
{
public:
    virtual ~CLuaUserData() {}

    enum class Type
    {
        VECTOR2D,
        VECTOR3D,
        VECTOR4D,
        MATRIX,
        ELEMENT,
        RESOURCE,
        LUA_TIMER,
        XML_NODE
    };

    //virtual Type GetType() = 0;
};

class CLuaUserDataVector4D : protected CLuaUserData, public CVector4D
{
public:
    CLuaUserDataVector4D(const CVector4D& value = {}) : CVector4D(value) {}

    CVector4D& Get() { return *this; }
    const CVector4D& Get() const { return *this; }

    //virtual Type GetType() override { return CLuaUserData::Type::VECTOR4D; }
};

class CLuaUserDataVector3D : protected CLuaUserData, public CVector
{
public:
    CLuaUserDataVector3D(const CVector& value = {}) : CVector(value) {}

    CVector& Get() { return *this; }
    const CVector& Get() const { return *this; }

    //virtual Type GetType() override { return CLuaUserData::Type::VECTOR3D; }
};

class CLuaUserDataVector2D : protected CLuaUserData, public CVector2D
{
public:
    CLuaUserDataVector2D(const CVector2D& value = {}) : CVector2D(value) {}

    CVector2D& Get() { return *this; }
    const CVector2D& Get() const { return *this; }

    //virtual Type GetType() override { return CLuaUserData::Type::VECTOR2D; }
};

class CLuaUserDataMatrix : protected CLuaUserData, public CMatrix
{
public:
    CLuaUserDataMatrix(const CMatrix& value = {}) : CMatrix(value) {}

    CMatrix& Get() { return *this; }
    const CMatrix& Get() const { return *this; }

    //virtual Type GetType() override { return CLuaUserData::Type::MATRIX; }
};

class CLuaUserDataElement : protected CLuaUserData, ElementID
{
public:
    CLuaUserDataElement(const ElementID& value = INVALID_ELEMENT_ID) : ElementID(value) {}

    // Cast to required type
    template<class ExpectedT>
    ExpectedT Get() const
    {
        auto pEntity = CElementIDs::GetElement(*this); // Sadly we still need to use this, but its fast (in theory lol)
        if (!pEntity || pEntity->IsBeingDeleted() || !pEntity->IsA(ExpectedT::GetClassId()))
            return nullptr;
        return pEntity;  
    }

    template<class ExpectedT>
    operator ExpectedT() const { return Get<ExpectedT>(); }

    //virtual Type GetType() override { return CLuaUserData::Type::ELEMENT; }
};

class CLuaUserDataResource : protected CLuaUserData, ElementID
{
public:
    CLuaUserDataResource(ElementID resourceID) : ElementID(resourceID) {}

    CResource* Get() const
    {
        return g_pClientGame->GetResourceManager()->GetResourceFromScriptID(Value());
    }

    operator CResource*() const { return Get(); }

    //virtual Type GetType() override { return CLuaUserData::Type::RESOURCE; }
};

class CLuaUserDataLuaTimer : protected CLuaUserData, ElementID
{
public:
    CLuaUserDataLuaTimer(ElementID value) : ElementID(value) {}

    CLuaTimer* Get(lua_State* luaVM) const
    {
        if (CLuaMain* pLuaMain = CLuaDefs::m_pLuaManager->GetVirtualMachine(luaVM))
            return pLuaMain->GetTimerManager()->GetTimerFromScriptID(Value());
        return nullptr;
    }

    //virtual Type GetType() override { return CLuaUserData::Type::LUA_TIMER; }
};

class CLuaUserDataXMLNode : protected CLuaUserData, ElementID
{
public:
    CLuaUserDataXMLNode(ElementID value) : ElementID(value) {}

    CXMLNode* Get() const
    {
        return g_pCore->GetXML()->GetNodeFromID(Value());
    }

    operator CXMLNode*() const { return Get(); }

    //virtual Type GetType() override { return CLuaUserData::Type::XML_NODE; }
};
