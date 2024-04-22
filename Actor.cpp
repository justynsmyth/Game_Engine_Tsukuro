#include "Actor.h"
#include "ComponentManager.h"
#include <algorithm>


void Actor::AttachComponent(const std::string& componentKey, luabridge::LuaRef& componentInstance) {
    // Check to make sure that a component has a type variable associated with it
    assert(!componentInstance.isNil());
    //assert(componentInstance.isTable());
    assert(componentInstance["type"].isString());
	if (components.find(componentKey) == components.end()) {
		components.insert({ componentKey, componentInstance });
	}
}

void Actor::Start(bool init_ready) {
    if (OnStartActivated) {
        return;
    }
    for (auto& [key, componentRef] : this->GetAllComponents()) {
        if (!(componentRef)["enabled"]) {
            return;
        }
        // Check if the 'OnStart' function is present in this component
        if ((componentRef)["OnStart"].isFunction()) {
            try {
                // Call the 'OnStart' method, passing the component instance as 'self'
                (componentRef)["OnStart"]((componentRef));
                OnStartActivated = true;
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
        if (init_ready && (componentRef)["Ready"].isFunction()) {
            try {
                // Call the 'Ready' method, passing the component instance as 'self'
                (componentRef)["Ready"](componentRef);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << (this->name) << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::Update() {
    for (auto& [key, componentRef] : this->GetAllComponents()) {
        if (!(componentRef)["enabled"]) {
            continue;
        }
        // Check if the 'OnUpdate' function is present in this component
        if ((componentRef)["OnUpdate"].isFunction()) {
            try {
                // Call the 'OnStart' method, passing the component instance as 'self'
                (componentRef)["OnUpdate"]((componentRef));
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::LateUpdate() {
    for (auto& [key, componentRef] : this->GetAllComponents()) {
        if (!(componentRef)["enabled"]) {
            continue;
        }
        // Check if the 'OnLateUpdate' function is present in this component
        if ((componentRef)["OnLateUpdate"].isFunction()) {
            try {
                // Call the 'OnLateUpdate' method, passing the component instance as 'self'
                (componentRef)["OnLateUpdate"]((componentRef));
            }
            catch (luabridge::LuaException const& e) {
                ReportError(this->GetName(), e);
            }
        }
    }
}

void Actor::ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string error_message = e.what();
    std::replace(error_message.begin(), error_message.end(), '\\', '/');
    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}


void Actor::InjectConvenienceReferences(luabridge::LuaRef& component_ref) {
    (component_ref)["actor"] = this;
}

luabridge::LuaRef Actor::GetComponentByKey(const std::string& componentKey) {
    auto search = components.find(componentKey);
    if (search != components.end()) {
        return (search->second);
    }
    return luabridge::LuaRef(ComponentManager::GetLuaState());
}

/// <summary>
/// Loops through all components to find matching componentType
/// If componentType matches, determine if component is in process of being removed (return Nil)
/// if not, return component
/// </summary>
/// <param name="componentType"></param>
/// <returns>LuaRef Object</returns>
luabridge::LuaRef Actor::GetComponent(const std::string& componentType) {
    for (const auto& pair : components) {
        luabridge::LuaRef component = pair.second;
        if (!component.isNil()) {
            if (component.isTable()) {
                // Existing logic for components stored as tables in Lua
                luabridge::LuaRef typeField = component["type"];
                if (!typeField.isNil() && typeField.isString() && typeField.cast<std::string>() == componentType) {
                    if (removed_components.find(component["key"]) == removed_components.end()) {
                        return component;
                    }
                }
            }
            else if (component.isUserdata()) {
                // Logic for components stored as userdata in Lua
                // Assuming you have bound a GetType method in your Rigidbody class
                luabridge::LuaRef typeField = component["type"];
                std::string typeName = typeField.cast<std::string>();
                if (typeName == componentType) {
                    return component;
                }
            }
        }
    }
    // return Nil
    return luabridge::LuaRef(ComponentManager::GetLuaState());
}


/* 
Finds all components that have component['type'] = componentType
Returns it in a LuaRef Table form that is iterable with ipairs()
*/
luabridge::LuaRef Actor::GetComponents(const std::string& componentType) const {
    luabridge::LuaRef componentsTable = luabridge::newTable(ComponentManager::GetLuaState());
    int idx = 1;

    for (const auto& pair : components) {
        luabridge::LuaRef component = pair.second;
        if (!component.isNil() && component.isTable()) {
            luabridge::LuaRef typeField = component["type"];
            if (!typeField.isNil() && typeField.isString() && typeField.cast<std::string>() == componentType) {
                componentsTable[idx++] = component;
            }
        }
    }
    return componentsTable;
}


luabridge::LuaRef Actor::LuaAddComponent(const std::string& componentType) {
    if (global_component_counter.find(componentType) == global_component_counter.end()) {
        global_component_counter[componentType] = 0;
    }
    else {
        global_component_counter[componentType]++;
    }

    std::string componentKey = "r" + std::to_string(global_component_counter[componentType]);

    luabridge::LuaRef componentInstance = GetComponentByKey(componentKey);

    if (componentType == "Rigidbody") {
        componentInstance = ComponentManager::CreateRigidbody();
        componentInstance["type"] = componentType;
        componentInstance["enabled"] = true;
        componentInstance["key"] = componentKey;
        added_components.emplace(componentKey, componentInstance);
    }
    if (componentInstance.isNil() && !componentType.empty()) {
        componentInstance = ComponentManager::CreateComponentInstance(componentType);
        componentInstance["type"] = componentType;
        componentInstance["enabled"] = true;
        componentInstance["key"] = componentKey;
        added_components.emplace(componentKey, componentInstance);
        InjectConvenienceReferences(componentInstance);
    }
    return componentInstance;
}

void Actor::LuaRemoveComponent(const luabridge::LuaRef& component_ref) {

    for (auto it = components.begin(); it != components.end(); ++it) {
        if (it->second == component_ref) {
            it->second["enabled"] = false;
            removed_components.insert({ it->first, it->second });
        }
    }
    return;
}

void Actor::ProcessAddedComponents() {
    for (auto& it : added_components) {
        AttachComponent(it.first, it.second);
        auto componentRef = it.second;
        if (!(componentRef)["enabled"]) {
            continue;
        }
        // Check if the 'OnStart' function is present in this component
        if ((componentRef)["OnStart"].isFunction()) {
            try {
                // Call the 'OnStart' method, passing the component instance as 'self'
                (componentRef)["OnStart"]((componentRef));
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << this->name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
        if ((componentRef)["Ready"].isFunction()) {
            try {
                // Call the 'Ready' method, passing the component instance as 'self'
                (componentRef)["Ready"](componentRef);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << (this->name) << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
    added_components.clear();
}

void Actor::ProcessRemovedComponents() {
    for (auto& it : removed_components) {
        RemoveComponent(it.second);
    }
    removed_components.clear();
}

void Actor::RemoveComponent(luabridge::LuaRef& component_ref) {
    if (component_ref["OnDestroy"].isFunction()) {
        try {
            component_ref["OnDestroy"](component_ref);
        }
        catch (const luabridge::LuaException& e) {
            std::cout << "Error while calling OnDestroy: " << e.what() << std::endl;
        }
    }
    if (component_ref.isInstance<Rigidbody>()) {
        Rigidbody* rigidbody = component_ref.cast<Rigidbody*>();
        if (rigidbody) {
            rigidbody->OnDestroy();  // this will clean up and delete the instance
        }
    }
    component_ref["enabled"] = false;
    for (auto it = components.begin(); it != components.end(); ++it) {
        if (it->second == component_ref) {
            components.erase(it);
            break;
        }
    }
}

void Actor::OnCollisionEnter(Collision* collision)
{
    for (auto& [key, component] : this->GetAllComponents()) {
        if (component["OnCollisionEnter"].isFunction()) {
            try {
                component["OnCollisionEnter"](component, collision);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::OnTriggerEnter(Collision* collision)
{
    for (auto& [key, component] : this->GetAllComponents()) {
        if (component["OnTriggerEnter"].isFunction()) {
            try {
                component["OnTriggerEnter"](component, collision);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::OnCollisionExit(Collision* collision)
{
    collision->point = b2Vec2(-999.0f, -999.0f);
    collision->normal = b2Vec2(-999.0f, -999.0f);
    for (auto& [key, component] : this->GetAllComponents()) {
        if (component["OnCollisionExit"].isFunction()) {
            try {
                component["OnCollisionExit"](component, collision);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::OnTriggerExit(Collision* collision)
{
    for (auto& [key, component] : this->GetAllComponents()) {
        if (component["OnTriggerExit"].isFunction()) {
            try {
                component["OnTriggerExit"](component, collision);
            }
            catch (luabridge::LuaException const& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
}

void Actor::OnDestroy() {
    for (auto& [key, componentRef] : components) {
        if (componentRef["OnDestroy"].isFunction()) {
            try {
                componentRef["OnDestroy"](componentRef);
            }
            catch (const luabridge::LuaException& e) {
                std::cout << "\033[31m" << name << " : " << e.what() << "\033[0m" << std::endl;
            }
        }
    }
    for (auto it = components.begin(); it != components.end(); ++it) {
        removed_components.insert({ it->first, it->second });
    }
}

void Actor::DisableAllComponents() {
    for (auto it = components.begin(); it != components.end(); ++it) {
        it->second["enabled"] = false;
    }
}
