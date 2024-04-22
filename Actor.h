//
//  Actor.h
//  game_engine
//
//  Created by Justin Smith on 2/4/24.
//

#ifndef Actor_h
#define Actor_h

#ifdef _WIN32
    #include "ThirdParty/SDL/include/SDL.h"
#elif __APPLE__
    #include "framework/SDL2.framework/Headers/SDL.h"
#else
    #include <SDL2/SDL.h>
#endif
#include <optional>
#include "ThirdParty/glm-0.9.9.8/glm/glm.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include "ThirdParty/Lua/src/lua.hpp"
#include "ThirdParty/LuaBridge/LuaBridge.h"

class Component;
class Collision;

class Actor
{
private:
    static int nextId;
    static inline std::unordered_map<std::string, int> global_component_counter;
    std::map<std::string, luabridge::LuaRef> components;
    std::map<std::string, luabridge::LuaRef> added_components;
    std::map<std::string, luabridge::LuaRef> removed_components;
public:

    int id;
    std::string name;
    bool DontDestroy;
    bool OnStartActivated;

    Actor() :
        id(nextId++),
        name(""),
        DontDestroy(false),
        OnStartActivated(false)
    {}

    Actor(const std::string& name)
        : id(nextId++),
        name(name),
        DontDestroy(false),
        OnStartActivated(false)
    {}

    Actor(const Actor& other) : id(nextId++),
        name(other.name),
        DontDestroy(other.DontDestroy),
        OnStartActivated(other.OnStartActivated)
    {}

    void AttachComponent(const std::string& componentKey, luabridge::LuaRef& componentInstance);
    void RemoveComponent(luabridge::LuaRef& component_ref);
    void OnCollisionEnter(Collision* collision);
    void OnTriggerEnter(Collision* collision);
    void OnCollisionExit(Collision* collision);
    void OnTriggerExit(Collision* collision);

    std::string GetName() const {
        return name;
    }
    int GetID() const {
        return id;
    }
    luabridge::LuaRef GetComponentByKey(const std::string& componentKey);
    luabridge::LuaRef GetComponent(const std::string& componentType);
    luabridge::LuaRef GetComponents(const std::string& componentType) const;


    std::map<std::string, luabridge::LuaRef> GetAllComponents() const {
        return components;
    }

    void InjectConvenienceReferences(luabridge::LuaRef& component_ref);
    luabridge::LuaRef LuaAddComponent(const std::string& componentType);
    void LuaRemoveComponent(const luabridge::LuaRef& component_ref);

    void ReportError(const std::string& actor_name, const luabridge::LuaException& e);
    void ProcessAddedComponents();
    void ProcessRemovedComponents();

    void Start(bool init_ready);
    void Update();
    void LateUpdate();
    void OnDestroy();
    void DisableAllComponents();

};

#endif /* Actor_h */
