#include <iostream>
#include <filesystem>
#include <cstdlib>
#include "GameManager.h"
#include "TemplateDB.h"

namespace fs = std::filesystem;

TemplateDataBase::TemplateDataBase(ComponentManager& m) :
    componentManager(&m)
{}

std::shared_ptr<Actor> TemplateDataBase::getTemplate(const std::string& s, GameManager& game) {
    std::string templatePath = "resources/actor_templates/" + s + ".template";
    if (!fs::exists(templatePath)) {
        std::cerr << "error: template " << s << " is missing" << std::endl;
        exit(0);
    }

    // If the template exists, return a clone of template
    if (templates.find(s) != templates.end()) {
        return cloneActor(templates[s]);
    }

    // Template DNE, load and parse the template file
    rapidjson::Document templateDoc;
    game.ReadJsonFile(templatePath, templateDoc);

    auto actor = parseActorFromJson(templateDoc);

    templates[s] = actor;

    return cloneActor(actor);
}

std::shared_ptr<Actor> TemplateDataBase::cloneActor(const std::shared_ptr<Actor>& originalActor) {
    // Create a new actor instance and copy basic actor data
    auto clonedActor = std::make_shared<Actor>();
    clonedActor->name = originalActor->name;

    // Copy and clone each component
    for (auto& componentPair : originalActor->GetAllComponents()) {
        auto key = componentPair.first;
        auto& component = componentPair.second;
        if (component.isInstance<Rigidbody>()) {
            luabridge::LuaRef componentInstance = ComponentManager::CreateRigidbody();
            componentInstance["enabled"] = true;
            std::string componentType = "Rigidbody";
            componentInstance["type"] = componentType;
            Rigidbody* originalRb = component.cast<Rigidbody*>();
            Rigidbody* clonedRb = componentInstance.cast<Rigidbody*>();
            clonedRb->copyParameters(*originalRb);
            luabridge::LuaRef componentRef(ComponentManager::GetLuaState(), clonedRb);
            clonedActor->InjectConvenienceReferences(componentRef);
            clonedActor->AttachComponent(key, componentRef);
        }
        else {
            auto clonedComponent = componentManager->CloneComponentInstance(component);
            clonedComponent["enabled"] = true;
            clonedActor->InjectConvenienceReferences(clonedComponent);
            clonedActor->AttachComponent(key, clonedComponent);
        }
    }
    return clonedActor;
}

std::shared_ptr<Actor> TemplateDataBase::parseActorFromJson(const rapidjson::Document& json) {
    auto actor = std::make_shared<Actor>();

    if (json.HasMember("name")) {
        actor->name = json["name"].GetString();
    }

    if (json.HasMember("components")) {
        for (const auto& componentEntry : json["components"].GetObject()) {
            std::string componentKey = componentEntry.name.GetString();
            const rapidjson::Value& componentValue = componentEntry.value;
            std::string componentType = componentValue["type"].GetString();
            if (!componentManager->find_component(componentType) && componentType != "Rigidbody") {
                std::cerr << "error: failed to locate component " << componentType << "\n";
                exit(0);
            }
            luabridge::LuaRef componentInstance = luabridge::LuaRef(ComponentManager::GetLuaState());
            if (componentType == "Rigidbody") {
                componentInstance = ComponentManager::CreateRigidbody();
            }
            else {
                componentInstance = componentManager->CreateComponentInstance(componentType);
            }
            
            // create a base template version of component
            for (auto it = componentValue.MemberBegin(); it != componentValue.MemberEnd(); ++it) {
                std::string nameString(it->name.GetString());
                if (nameString == "type") continue;

                const auto& memberValue = it->value;
                if (memberValue.IsString()) {
                    (componentInstance)[nameString] = memberValue.GetString();
                }
                else if (memberValue.IsFloat()) {
                    (componentInstance)[nameString] = memberValue.GetFloat();
                }
                else if (memberValue.IsInt()) {
                    (componentInstance)[nameString] = memberValue.GetInt();
                }
                else if (memberValue.IsBool()) {
                    componentInstance[nameString] = memberValue.GetBool();
                }
            }
            // If Rigidbody, we do not need to make a clone.
            if (componentInstance["Ready"].isFunction()) {
                componentInstance["enabled"] = true;
                componentInstance["key"] = componentKey;
                componentInstance["type"] = componentType;
                actor->AttachComponent(componentKey, componentInstance);
                continue;
            }
            // add instance based information here
            auto newComponentRef = componentManager->CloneComponentInstance(componentInstance);
            newComponentRef["key"] = componentKey;
            newComponentRef["type"] = componentType;
            actor->AttachComponent(componentKey, newComponentRef);
        }
    }
    return actor;
}
