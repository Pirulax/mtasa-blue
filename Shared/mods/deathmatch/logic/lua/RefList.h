#pragma once

#include <vector>

struct lua_State;

namespace lua
{
// Creates and manages lifetime of references
struct RefList
{
    RefList(lua_State* L, size_t initialCapacity = 1024) : m_lua(L)
    {
        m_refs.reserve(initialCapacity);
    }

    ~RefList();

    // Creates a new refernece for the value at top of
    // the stack and pops it (unless keepValue is true)
    [[nodiscard]] int Create(bool keepValue = false);

    // Puts value at top of the stack
    // ref should be a value returned by `Create`
    void Get(int ref);

    // For range stuff
    auto begin() const { return m_refs.begin(); }
    auto end() const { return m_refs.end(); }

    // Might be confusing? One might try to use the returned refs
    //int operator[](size_t i) const { return m_refs[i]; }

protected:
    std::vector<int> m_refs; // Keep track of refs
    lua_State* m_lua;
};
};
