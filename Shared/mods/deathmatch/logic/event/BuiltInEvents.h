#pragma once

#include "Event.h"
#include "BuiltInEventsListMacroApply.h"

// How to add a new event: Go to BuiltInEventsListMacroApply.h

class BuiltInEvents
{
public:
    // These variables have external linkage, so their addresses are the same across all TUs(.cpp files)
    // This makes it possible to use their address (eg.: Event*) as the key to hash maps
#define DECLARE_EVENT(name, id) inline static const BuiltInEvent name{#name, BUILTINEVENT_ID_EXPAND(id)};
    BUILTINEVENT_LIST_APPLY_MACRO(DECLARE_EVENT)
#undef DECLARE_EVENT

    inline static constexpr auto COUNT{ static_cast<size_t>(BuiltInEvent::ID::COUNT) };
};

#undef BUILTINEVENT_LIST_APPLY_MACRO
#undef BUILTINEVENT_ID_EXPAND