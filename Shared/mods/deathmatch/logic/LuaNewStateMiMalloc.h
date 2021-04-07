#pragma once
#include <iostream>
#include <mimalloc.h>

/* this is copied from Lua, just changed fput(stderr) to std::cerr */
int LuaPanic(lua_State* L) {
    std::cerr << "PANIC: unprotected error in call to Lua API " << lua_tostring(L, -1) << std::endl;
    return 0;
}

void* LuaMiMalloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize;
    if (nsize == 0) {
        mi_free(ptr);
        return NULL;
    }
    else
        return mi_realloc(ptr, nsize);
}

lua_State* LuaNewStateMiMalloc(class CLuaMain* owner)
{
    return lua_newstate(&LuaMiMalloc, NULL, static_cast<void*>(owner));
}
