/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/deathmatch/logic/CIdArray.h
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/
#pragma once

/*

    Lists the script object types used. The number represents the maximum amount amount if unique IDs.
    CXMLArray is still using the old fashioned ID system... (so it'll leak over time)
    The maximum number unique IDs for each type can be found in the `GUID::MAX_INDEX` variable

    Server only object types:
        CAccount                 
        CAccessControlListGroup
        CAccessControlList     
        CBan
        CDbJobData
        CTextItem              
        CTextDisplay

    Client:
        - Has no custom types

    Shared object types(So these types are present at both the client and server):
        CXMLNode                        CXMLArray - 16,777,216
        CElement (aka CClientEntity)    CElementIDs - 131,072 shared(client and server), and 131,072 client only
        CResource
        CLuaMatrix
        CLuaTimer
        CLuaVector2D
        CLuaVector3D
        CLuaVector4D

*/
#include <climits>
#include <array>
#include <memory>
#include <vector>
#include <unordered_map>
#include <numeric>
#include "../XML/CXMLArray.h"

namespace ScriptObject
{
    class CXMLNode;
    class CResource;
    class CLuaTimer;
    class CLuaVector2D;
    class CLuaVector3D;
    class CLuaVector4D;
    class CLuaMatrix;
    class CAccount;
    class CAccessControlList;
    class CAccessControlListGroup;
    class CBan;
    class CDbJobData;
    class CTextDisplay;
    class CTextItem;

#ifdef MTA_CLIENT // Client has CClientEntity instead of CElement
    class CClientEntity;
    using CElement = CClientEntity;
#else
    class CElement;
#endif

#pragma region Type
    // Type of the object this GUID referes to
    enum class Type : unsigned char
    {
        // Shared:
        ELEMENT,
        XML_NODE,
        RESOURCE,
        TIMER,
        VECTOR2,
        VECTOR3,
        VECTOR4,
        MATRIX,
#ifndef MTA_CLIENT // Server only
        ACCOUNT,
        ACL,
        ACL_GROUP,
        BAN,
        DB_JOBDATA,
        TEXT_DISPLAY,
        TEXT_ITEM,
#endif
        // Special types, should not be used

        NUM,
        FIRST = 0,
        LAST = NUM - 1,
        BEGIN = FIRST,
        END = NUM // Past the end, as in `std::vector::end()`
    };
#pragma region TypeTraits
#pragma region ClassToType
    template<class T>
    static constexpr Type ClassToType();
    template<>
    static constexpr Type ClassToType<CElement>() { return Type::ELEMENT; }
    template<>
    static constexpr Type ClassToType<CXMLNode>() { return Type::XML_NODE; }
    template<>
    static constexpr Type ClassToType<CResource>() { return Type::RESOURCE; }
    template<>
    static constexpr Type ClassToType<CLuaTimer>() { return Type::TIMER; }
    template<>
    static constexpr Type ClassToType<CLuaVector2D>() { return Type::VECTOR2; }
    template<>
    static constexpr Type ClassToType<CLuaVector3D>() { return Type::VECTOR3; }
    template<>
    static constexpr Type ClassToType<CLuaVector4D>() { return Type::VECTOR4; }
    template<>
    static constexpr Type ClassToType<CLuaMatrix>() { return Type::MATRIX; }
#ifndef MTA_CLIENT
    template<>
    static constexpr Type ClassToType<CAccount>() { return Type::ACCOUNT; }
    template<>
    static constexpr Type ClassToType<CAccessControlList>() { return Type::ACL; }
    template<>
    static constexpr Type ClassToType<CAccessControlListGroup>() { return Type::ACL_GROUP; }
    template<>
    static constexpr Type ClassToType<CBan>() { return Type::BAN; }
    template<>
    static constexpr Type ClassToType<CDbJobData>() { return Type::DB_JOBDATA; }
    template<>
    static constexpr Type ClassToType<CTextDisplay>() { return Type::TEXT_DISPLAY; }
    template<>
    static constexpr Type ClassToType<CTextItem>() { return Type::TEXT_ITEM; }
#endif
#pragma endregion

#pragma region TypeToClass
    template<Type>
    struct TypeToClass;
    template<>
    struct TypeToClass<Type::ELEMENT> { using type = CElement; };
    template<>
    struct TypeToClass<Type::XML_NODE> { using type = CXMLNode; };
    template<>
    struct TypeToClass<Type::RESOURCE> { using type = CResource; };
    template<>
    struct TypeToClass<Type::TIMER> { using type = CLuaTimer; };
    template<>
    struct TypeToClass<Type::VECTOR2> { using type = CLuaVector2D; };
    template<>
    struct TypeToClass<Type::VECTOR3> { using type = CLuaVector3D; };
    template<>
    struct TypeToClass<Type::VECTOR4> { using type = CLuaVector4D; };
    template<>
    struct TypeToClass<Type::MATRIX> { using type = CLuaMatrix; };
#ifndef MTA_CLIENT
    template<>
    struct TypeToClass<Type::ACCOUNT> { using type = CAccount; };
    template<>
    struct TypeToClass<Type::ACL> { using type = CAccessControlList; };
    template<>
    struct TypeToClass<Type::ACL_GROUP> { using type = CAccessControlListGroup; };
    template<>
    struct TypeToClass<Type::BAN> { using type = CBan; };
    template<>
    struct TypeToClass<Type::DB_JOBDATA> { using type = CDbJobData; };
    template<>
    struct TypeToClass<Type::TEXT_DISPLAY> { using type = CTextDisplay; };
    template<>
    struct TypeToClass<Type::TEXT_ITEM> { using type = CTextItem; };
#endif
#pragma endregion
#pragma endregion

#pragma endregion

    // A globally unique identifer for a script object
    // Can be stored in a Lua `light userdata`, or a `full userdata`
    // When stored in Lua its `Compressed` (into a void* / size_t)
    struct GUID
    {
        typedef size_t Compressed;

    #pragma region Constructors
        constexpr GUID() : GUID(INVALID_GUID) {};
        GUID() = default;

        constexpr GUID(Compressed type) :
            index(type >> BITS_TO_REPRESENT_TYPE), type(Type(type & TYPE_BITMASK)) {}

        explicit GUID(ElementID elemID) :
            type(Type::ELEMENT), index(elemID.Value()) {}

        constexpr GUID(Type type, size_t index) :
            type(type), index(index) {}

        static GUID FromLightUserData(void* userdata)
        {
            return GUID((Compressed)userdata);
        }

        static GUID FromFullUserData(void* userdata)
        {
            return GUID(*((Compressed*)userdata));
        }
    #pragma endregion

    #pragma region Methods
        Compressed Compress() const
        {
            dassert(MAX_INDEX > index);
            dassert((size_t)Type::LAST >= (size_t)type);

            Compressed compressed = 0;
            compressed |= index << BITS_TO_REPRESENT_TYPE; // Set high bits to `index`
            compressed |= (size_t)type; // Set lower bits to `type`

            return compressed;
        }
    
        template<class T>
        T* Get() const { return GUIDManager::Get<T>(*this);  }

        // Calls the lambda with the type of the guid.
        // Example: If type == Type::ACCOUNT:
        // The given `visitor` will be called with the type `CAccount*`
        // The pointer will be null if the GUID is invalid (the entry doesnt exist anymore)
        template<class Lambda_t>
        void Visit(const Lambda_t& visitor);
    #pragma endregion

    #pragma region Operators
        bool operator==(const GUID& rhs) { return rhs.type == type && rhs.index == index; }
        bool operator!=(const GUID& rhs) { return !(*this == rhs); }
    #pragma endregion

#pragma region 
    public:
        size_t index;
        Type   type;

    private:
        // Constants for Compressed GUID

        // Number of bits needed to represent `Type`
        static constexpr size_t BITS_TO_REPRESENT_TYPE = NumberOfSignificantBits<(size_t)Type::LAST>::COUNT;
        static constexpr size_t TYPE_BITMASK = ((size_t)1u << BITS_TO_REPRESENT_TYPE) - 1u;

        // Number of bits needed to represent `Index`
        static constexpr size_t BITS_TO_REPRESENT_INDEX = sizeof(size_t) * CHAR_BIT - BITS_TO_REPRESENT_TYPE;
        static constexpr size_t MAX_INDEX = ((size_t)1u << BITS_TO_REPRESENT_INDEX) - 1u;

        // Check if fits exactly into a size_t
        static_assert(BITS_TO_REPRESENT_INDEX + BITS_TO_REPRESENT_TYPE == sizeof(size_t) * CHAR_BIT);
        static_assert((MAX_INDEX << BITS_TO_REPRESENT_TYPE) | TYPE_BITMASK == std::numeric_limits<size_t>::max());
        static_assert(sizeof(size_t) == sizeof(void*)); // And if a size_t fits into a void*
    }; // struct GUID
    static constexpr auto INVALID_GUID = GUID(Type::END, -1u);

    // Like a `smart pointer`. Automatically Allocates and Frees the object GUID
    // Also adds the `GetScriptObjectGUID` method for convinience.
    template<class T>
    class AutoGUID
    {
    protected:
        // Can only be constructed by a class we're a base of
        // it doesn't make sense to constructr it outside a class
        AutoGUID();
        ~AutoGUID();

        void FreeGUID();

    public:
        GUID GetScriptObjectGUID() const { return m_scriptObjectGUID; }

        AutoGUID& operator=(AutoGUID&& rhs)
        {
            if (this != &rhs)
            {
                FreeGUID(); // Deallocate ID before assigning a new one
                m_scriptObjectGUID = rhs.m_scriptObjectGUID;
                rhs.m_scriptObjectGUID = INVALID_GUID; // Make sure the ID doesn't get free'd
            }
            return *this;
        }

        AutoGUID& operator=(const AutoGUID& rhs) = delete; // Can only be moved
    protected:
        GUID m_scriptObjectGUID = INVALID_GUID;
    };

    // Like CSingleton, but can be Inited / Deinited on demand
    template <typename T>
    class ManagedSingleton
    {
    public:
        static void Initialize()
        {
            dassert(!m_instance);
            m_instance = new T;
        }

        static void Deinitialize()
        {
            dassert(m_instance);
            delete m_instance;
            m_instance = nullptr;
        }

        static T* GetInstance()
        {
            dassert(m_instance);
            return m_instance;
        }

    protected:
        static T* m_instance;
    };

    template<class T>
    class GUIDBucket
    {
        friend class IDManager<T>;
    protected:
        static constexpr size_t GetSize() { return 4096; }

        size_t GetCurrentyInUse() const { return m_currInUseCount; }
        size_t GetNeverUsedCount() const { return GetSize() - m_nextIDToUse - 1; }
        size_t GetUnusableCount() const { return GetSize() - GetNeverUsedCount() - GetCurrentyInUse(); }

        bool HasAnyInUse() const { return m_usedCount != 0; }
        bool HasUniqueLeft() const { return m_nextIDToUse < m_items.size(); }

        auto begin() const { return m_items.begin(); }
        auto end() const { return m_items.end(); }

        T* Get(size_t index)
        {
            dassert(index >= m_nextIDToUse); // Assert if an non allocated ID comes along..
            return m_items[index]; // Can be nullptr
        }

        size_t Allocate(T* object)
        {
            dassert(!m_items[m_nextIDToUse]);
            dassert(HasUniqueLeft()); // Caller should check for this

            m_currInUseCount++;
            m_items[m_nextIDToUse] = object;
            return m_nextIDToUse++;
        }

        void Free(size_t index, T* object)
        {
            dassert(m_items[index] == object);
            m_items[index] = nullptr; // Needs to be nulled, so `Get` returns null
            m_currInUseCount--;
        }

    private:
    #ifdef MTA_DEBUG
        std::array<T*, GetSize()> m_items = { nullptr };
    #else
        std::array<T*, GetSize()> m_items;
    #endif

        size_t m_nextIDToUse = 0;
        size_t m_currInUseCount = 0;
    };

    // Handles per-type ID (Type as a class type from the `Type` enum)
    template<class T>
    class IDManager : ManagedSingleton<IDManager<T>>
    {
        friend class GUIDManager;

    public:
        using value_type = T;

    protected:
        // Tries to free unused buckets (by deallocating buckes with 0 used IDs)
        // `force`: forcefully deallocate buckets with `<= <forceMaxInUseAmount>` active IDs,
        //          and put their active IDs into the internal hashmap
        // targetDealloced: The target number of deallocated IDs
        size_t DeallocateUnusedBuckets(bool force = false, size_t forceMaxInUseAmount = 100u, size_t targetCountToDealloc = std::numeric_limits<size_t>::max())
        {
            size_t deallocatedCount = 0;
            for (size_t i = 0, size = m_buckets.size(); i < size; i++)
            {
                const auto& bucket = m_buckets[i];
                if (!bucket)
                    continue;

                if (bucket->HasUniqueLeft()) // Has free IDs?
                    continue;

                if (bucket->HasAnyInUse()) // Has IDs in use?
                {
                    if (!force) // Yes, but are we forced to deallocate?
                        continue;

                    if (bucket->GetCurrentyInUse() > forceMaxInUseAmount)
                        continue;

                    // Copy IDs into m_idmap
                    for (size_t i = 0; i < bucket->GetSize(); i++)
                    {
                        if (const T* ptr = bucket->Get(i)) // Only valid IDs
                        {
                            if (!std::get<bool>(m_idmap.emplace(i, ptr)))
                                dassert(0);// Assert if the ID is already in the map
                        }
                    }
                }

                deallocatedCount += bucket->GetUnusableCount();
                m_buckets[i] = nullptr; // Deallocate

                if (deallocatedCount >= targetCountToDealloc) // Check if dealloc count is reached
                    break;
            }
            return deallocatedCount;
        }

        T* Get(GUID guid)
        {
            if (guid.type != ClassToType<T>())
                return nullptr;

            // Find object in a bucket
            {
                const size_t bucketIndex = guid.index / GUIDBucket<T>::GetSize();
                dassert(m_buckets.size() > bucketIndex); // We never shrink, so if an index is out of range then has something went wrong

                const auto indexInBucket = guid.index % GUIDBucket<T>::GetSize();
                if (const auto& bucket = m_buckets[bucketIndex]) // Bucket can be null if it has been deallocated
                    if (T* ptr = bucket->Get(indexInBucket))
                        return ptr;
            }

            // Try hashtable
            if (const auto it = m_idmap.find(guid.index); it != m_idmap.end())
                return it->second;

            return nullptr; // Fuck it :D - Can happen!
        }

        GUID Allocate(T* object)
        {
            if (const auto& lastBucket = m_buckets.back(); lastBucket->HasUniqueLeft())
                return { ClassToType<T>(), lastBucket->Allocate(object) };

            // Emplace new bucket, and allocate there
            m_buckets.emplace_back(std::make_unique<GUIDBucket<T>>());
            return Allocate(object);
        }

        void Free(GUID guid, T* object)
        {
            dassert(guid.type == ClassToType<T>());

            // Search in buckets
            const size_t bucketIndex = guid.index / GUIDBucket<T>::GetSize();
            if (const auto& bucket = m_buckets[bucketIndex]) // Might be null if it got deallocated
                return bucket->Free(guid.index, object);

            else
            {
                // Then its in the map.
                const auto it = m_idmap.find(guid.index);
                dassert(it != m_idmap.end()); // Perhaps not?
                m_idmap.erase(it);
            }
        }

        // Returns the total number of unusable (Free'd) slots / ids (or however you call them)
        size_t GetUnusableCount()
        {
            return std::accumulate(m_buckets.begin(), m_buckets.end(), 0u,
                [](const auto& prev, const auto& bucket) {
                    if (!bucket)
                        return prev;
                    return prev + bucket->GetUnusableCount();
                }
            );
        }
    private:
        std::vector<std::unique_ptr<GUIDBucket<T>>> m_buckets;

        // IDs might end up in a map if a bucket only
        // had a few of them, and DeallocateUnusedBuckets with `force` was called
        // This is a tradeoff between memory and CPU time
        // TODO: Profile this. If on avarage there isnt a lot of element we can use
        // densehashmap to make it even faster. (Densehashmap uses 4x more memory)
        std::unordered_map<size_t, T*> m_idmap;
    };

    // CElement uses CElementIDs
    template<>
    class IDManager<class CElement>
    {
        friend class GUIDManager;

    public:
        using value_type = class CElement;

    protected:
        CElement* Get(GUID guid)
        {
            dassert(guid.type == ClassToType<CElement>());
            return (CElement*)CElementIDs::GetElement(guid.index);
        }
    };

    // CXMLNode uses CXMLArray
    // TODO: maybe refactor it one day..
    template<>
    class IDManager<class CXMLNode>
    {
        friend class GUIDManager;

    public:
        using value_type = class CXMLNode;

    protected:
        CXMLNode* Get(GUID guid)
        {
            dassert(guid.type == ClassToType<CXMLNode>());
            return (CXMLNode*)CXMLArray::GetEntry(guid.index);
        }
    };

    /*
    * The difference between `IDManager`, and this is that
    * an `IDManager` handles per-type ID, while this one
    * manages all of them.
    * 
    * This is the only Manager accessible to the public, it should be
    * called for Allocating / Freeing IDs, Get-ting pointers to objcets
    * by GUID, etc..
    */
    class GUIDManager
    {
    public:
        /*
        * Iterates through all GUID managers (Present in the `Type` enum)
        * It has no runtime cost, it's all inlined, just like you were to
        * write it by hand.
        * 
        * Note: IDManager for CElement and CXMLNode are just wrapper's around
        * their real ID array (CElementIDs and CXMLArray)
        *
        * If the passed in unary function returns `false` the iteration will
        * be halted (as in `break` in a loop)
        */
        template<bool IncludeXMLAndElement = true, auto ClassType = Type::BEGIN, class UnaryOp_t>
        static void IterManagers(const UnaryOp_t& unaryOp)
        {
            if constexpr (ClassType != Type::NUM)
            {
                if constexpr (IncludeXMLAndElement || (ClassType != Type::ELEMENT && ClassType != Type::XML_NODE))
                {
                    using Manager_t = IDManager<typename TypeToClass<ClassType>::type>;
                    const auto bindedUnaryOp = []
                        { return unaryOp(Manager_t::GetInstance(), ClassType); };

                    using UnaryFunctionRet_t = std::invoke_result_t<UnaryOp_t, Manager_t*, Type>;
                    if constexpr (std::is_same_v<UnaryFunctionRet_t, bool>) // Return type is bool?
                    {
                        if (!bindedUnaryOp()) // Yes, so if it returns `false` we stop the iteration
                            return;
                    }
                    else
                        bindedUnaryOp();
                }
                IterManagers<IncludeXMLNodeAndCElement, Type((size_t)ClassType + 1)>(lambda); // continue
            }
        }


        // (Client) Called when (re)connected to a server
        // (Server) Called when the server starts
        static void Initialize()
        {
            IterManagers<false>([](auto* mgr, Type type) {
                std::decay_t<decltype(mgr)>::Initialize();
            });
        }

        // (Client) Called when disconnected (or when reconnecting) from a server
        // (Server) Called when the server is stopped lol
        static void Deinitialize()
        {
            IterManagers<false>([](auto* mgr, Type type) {
                std::decay_t<decltype(mgr)>::Deinitialize();
            });
        }

        // Pulses the manager. If theres a lot of unused IDs then its going to
        // deallocate unused IDs (thus freeing memory)
        static void DoPulse()
        {
            static auto last = GetTickCount32();
            if (const auto now = GetTickCount32(); (now - last) > 30 * 1000)
            {
                last = now;

                size_t totalUnusable = 0;
                IterManagers<false>([&](auto* mgr, Type type) {
                    totalUnusable += mgr->GetUnusableCount();
                });

                IterManagers<false>([&totalUnusable](auto* mgr, Type type) {
                    totalUnusable -= mgr->DeallocateUnusedBuckets(); // Try deallocating normally first
                });


                for (size_t maxInUseCount = 0; totalUnusable > 0x4FFFFF; maxInUseCount += 5) // Still a lot of unusable IDs left?
                {
                    // Do until we have less than 0x4FFFFF
                    IterManagers<false>([&totalUnusable, maxInUseCount](auto* mgr, Type type) {
                        totalUnusable -= mgr->DeallocateUnusedBuckets(true, maxInUseCount);
                    });
                }
            }
        }

        // Allocates a new GUID for type T
        // `object` should point to an instance of T
        // (aka pass in `this` in T's member init list)
        template<class T>
        static GUID Allocate(T* object) { return IDManager<T>::GetInstance()->Allocate(object); }

        // Deallocates(frees) the given GUID which was allocated for type T
        // `object` should be `this` (aka a pointer to the object which called `Allocate` originally)
        template<class T>
        static void Free(GUID guid, T* object) { return IDManager<T>::GetInstance()->Free(guid, object); }

        // Returns a pointer to an object with type T with the given GUID
        // If T doesnt match with the type GUID refres to or
        // the GUID isn't valid anymore, it returns `nullptr`
        template<class T>
        static T* Get(GUID guid) { return IDManager<T>::GetInstance()->Get(guid); }
    };
};
