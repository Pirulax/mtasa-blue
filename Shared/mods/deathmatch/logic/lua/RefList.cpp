#include <StdInc.h>

#include "RefList.h"
extern "C"
{
    #include "lua.h"
}

namespace lua
{
RefList::~RefList()
{
    // Now invalidate these refs, so Lua can reuse them
    for (int ref : m_refs)
        lua_unref(m_lua, ref);
}

int RefList::Create(bool keepValue)
{
    if (keepValue)
        lua_pushvalue(m_lua, -1);
    return m_refs.emplace_back(lua_ref(m_lua, true));
}

void RefList::Get(int ref)
{
    lua_getref(m_lua, ref);
}
};


