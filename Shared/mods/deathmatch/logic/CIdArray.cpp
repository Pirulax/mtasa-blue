/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/deathmatch/logic/CIdArray.cpp
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#define SHARED_ARRAY_BASE_ID    0x02000000

namespace ScriptObject
{
    template<class Lambda_t>
    void GUID::Visit(const Lambda_t& visitor)
    {
        // This seems like O(n), but in reality... 
        // it really is, because MSVC and GCC are stupid,
        // only clang realized this can be a jmptbl... (https://godbolt.org/z/5KT73o)
        GUIDManager::IterManagers([this, &](auto* mgr, Type type) {
            if (this->type == type)
            {
                visitor(mgr->Get(index));
                return false; // Break
            }
            return true; // Continue
        });
    }

    template<class T>
    AutoGUID<T>::AutoGUID() :
        m_scriptObjectGUID(GUIDManager::Allocate(static_cast<T*>(this)))
    {
    }

    template<class T>
    AutoGUID<T>::~AutoGUID()
    {
        if (m_scriptObjectGUID != INVALID_GUID)
            FreeGUID();
    }

    template<class T>
    void AutoGUID<T>::FreeGUID()
    {
        dassert(m_scriptObjectGUID != INVALID_GUID);
        GUIDManager::Free(m_scriptObjectGUID, static_cast<T*>(this));   
    }
};


