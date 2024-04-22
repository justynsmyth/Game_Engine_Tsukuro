#pragma once
#include "ThirdParty/Lua/src/lua.hpp"
#include "ThirdParty/LuaBridge/LuaBridge.h"
class EventBus
{
public:
    using CallbackPair = std::pair<luabridge::LuaRef, luabridge::LuaRef>;
    using CallbackList = std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>;
    using EventMap = std::map<std::string, CallbackList>;

    static inline EventMap eventMap;
    static inline EventMap eventSubQueue;
    static inline EventMap eventUnsubQueue;

    static void ProcessSubscriptions() {
        for (const auto& pair : eventSubQueue) {
            auto& eventType = pair.first;
            auto& subs = pair.second;

            for (const auto& sub : subs) {
                eventMap[eventType].push_back(sub);
            }
        }
        eventSubQueue.clear();

        for (const auto& [eventType, unsubs] : eventUnsubQueue) {
            auto& existingSubs = eventMap[eventType];
            for (const auto& unsub : unsubs) {
                existingSubs.erase(std::remove_if(existingSubs.begin(), existingSubs.end(),
                    [&unsub](const CallbackPair& callback) {
                        return callback.first == unsub.first && callback.second == unsub.second;
                    }
                ), existingSubs.end());
            }
        }
        eventUnsubQueue.clear();
    }

    static void Publish(const std::string& event_type, const luabridge::LuaRef& event_object) {
        auto iter = eventMap.find(event_type);
        if (iter != eventMap.end()) {
            for (const auto& callbackPair : iter->second) {
                auto component = callbackPair.first;  // Lua table
                auto func = callbackPair.second;     // Lua function
                func(component, event_object);       // Call the function with (self, event_object)
            }
        }
    }

    static void Subscribe(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
        eventSubQueue[event_type].emplace_back(component, function);
    }

    static void Unsubscribe(const std::string& event_type, const luabridge::LuaRef& component, const luabridge::LuaRef& function) {
        eventUnsubQueue[event_type].emplace_back(component, function);
    }
};

