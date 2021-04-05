#pragma once

#include "BuiltInEvents.h"
#include "EventHandlerCollection.h"

#include <array>
#include <unordered_map>
#include <utility>

class CLuaMain;
class Event;
class CLuaFunctionRef;
class CustomEvent;
class BuiltInEvent;

class EventHandlerCallDispatcher
{
public:
    bool Remove(const Event& event, CLuaMain* lmain, const CLuaFunctionRef& fn);

    // This function is called to remove all handlers created by the given lmain.
    // Note: In theory, it's never called while we're processing an event
    // since resource stops are queued up, and executed at frame end
    void Remove(CLuaMain* lmain);

    void Remove(const CustomEvent& event);

    template<typename... Ts>
    void Emmit(const Event& event, Ts&&... ts)
    {
        if (auto handlers = GetHandlers(event))
            handlers->Emmit(event, std::forward<Ts>(ts)...);
    }

    auto Add(const Event& event, EventHandler handler) { return GetHandlers(event, true)->Add(handler); }

    EventHandlerCollection* GetHandlers(const Event& event, bool allowCreate = false);

    void Clear();
protected:

    auto GetHandlers(const BuiltInEvent& event)
    {
        return std::next(m_builtins.begin(), static_cast<size_t>(event.GetID()));
    }

    auto GetHandlers(const CustomEvent& event)
    {
        if (auto it = m_custom.find(&event); it != m_custom.end())
            return std::make_pair(it, true);
        return std::make_pair(m_custom.end(), false);
    }

    template<typename Fn>
    void ForAll(Fn fn)
    {
        for (auto& c : m_builtins)
            fn(c);
        for (auto& [k, c] : m_custom)
            fn(c);
    }

    std::array<EventHandlerCollection, BuiltInEvents::COUNT> m_builtins{};
    std::unordered_map<const CustomEvent*, EventHandlerCollection> m_custom;
};
