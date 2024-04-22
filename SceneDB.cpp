#include "SceneDB.h"
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <array>
#include <set>
namespace fs = std::filesystem;


int Actor::nextId = 0;
const std::array<glm::ivec2, 8> DIRECTIONS = {
        glm::ivec2(0, 1), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(-1, 0),
        glm::ivec2(1, 1), glm::ivec2(1, -1), glm::ivec2(-1, -1), glm::ivec2(-1, 1)
};


SceneDB::SceneDB(std::string& scenefile_path, rapidjson::Document& renderConfig,
                Renderer* RendererClass, rapidjson::Document& gameConfig,
                AudioDB& audioDB, ComponentManager& component) :
                RendererClass(RendererClass), gameConfig(gameConfig), audioDB(&audioDB), componentManager(&component) {
    // default size
    REGION_SIZE = { 0, 0, 100, 100 };
    initSceneActors(scenefile_path);

}

void SceneDB::StartScene(const std::string& scene_name) {
    current_scene = scene_name;
    for (auto& actor : scene_actors) {
        actor->Start(false);
    }
}

void SceneDB::initSceneActors(const std::string& scenefile_path) {
    rapidjson::Document doc;
    gameManager->ReadJsonFile(scenefile_path, doc);
    if (doc.HasMember("actors")) {
        const rapidjson::Value& actorsList = doc["actors"];
        scene_actors.reserve(actorsList.Size());
        for (rapidjson::SizeType i = 0; i < actorsList.Size(); i++) {
            addSceneActor(actorsList[i]);
        }
    }
}

void SceneDB::addSceneActor(const rapidjson::Value& actorJson) {

    std::shared_ptr<Actor> actor;

    if (actorJson.HasMember("template")) {
        std::string templateName = actorJson["template"].GetString();
        actor = templates->getTemplate(templateName, *gameManager);
    }
    else {
        actor = std::make_shared<Actor>();
    }
    setActorProperties(actorJson, actor);
    scene_actors.emplace_back(actor);
}




void SceneDB::setActorProperties(const rapidjson::Value& actorJson, std::shared_ptr<Actor>& actor) {
    actor->name = actorJson.HasMember("name")
        ? actorJson["name"].GetString() : actor->name;
    if (actorJson.HasMember("components")) {
        for (const auto& componentEntry : actorJson["components"].GetObject()) {
            std::string componentKey = componentEntry.name.GetString();
            const rapidjson::Value& componentValue = componentEntry.value;
            // get component type name.
            // Either a LuaComponent or a C++ Rigidbody component
            std::string componentType = "";
            if (componentValue.HasMember("type")) {
                componentType = componentValue["type"].GetString();
            }

            // See if component already exists inside of actor
            luabridge::LuaRef componentInstance = actor->GetComponentByKey(componentKey);

            if (componentType == "Rigidbody") {
                componentInstance = ComponentManager::CreateRigidbody();
                componentInstance["type"] = componentType;
                actor->AttachComponent(componentKey, componentInstance);
            }
            else if (!componentType.empty() && !componentManager->find_component(componentType)) {
                std::cout << "error: failed to locate component " << componentType;
                exit(0);
            }

            if (componentInstance.isNil() && !componentType.empty()) {
                componentInstance = componentManager->CreateComponentInstance(componentType);
                componentInstance["type"] = componentType;
                actor->AttachComponent(componentKey, componentInstance);
            }

            if (!componentInstance.isNil()) {
                componentInstance["enabled"] = true;
                componentInstance["key"] = componentKey;
                // add actor reference to instance
                actor->InjectConvenienceReferences(componentInstance);

                // Overrides anything in templates
                // Loop through and apply all properties from JSON, skipping the "type" property
                for (auto it = componentValue.MemberBegin(); it != componentValue.MemberEnd(); ++it) {
                    if (std::string(it->name.GetString()) == "type") continue;

                    const auto& memberName = it->name.GetString();
                    const auto& memberValue = it->value;

                    if (memberValue.IsString()) {
                        componentInstance[memberName] = memberValue.GetString();
                    }
                    else if (memberValue.IsFloat()) {
                        componentInstance[memberName] = memberValue.GetFloat();
                    }
                    else if (memberValue.IsInt()) {
                        componentInstance[memberName] = memberValue.GetInt();
                    } 
                    else if (memberValue.IsBool()) {
                        componentInstance[memberName] = memberValue.GetBool();
                    }
                }
            }

            if (componentInstance["Ready"].isFunction()) {
                try {
                    // Call the 'Ready' method, passing the component instance as 'self'
                    (componentInstance)["Ready"](componentInstance);
                }
                catch (luabridge::LuaException const& e) {
                    std::cout << "\033[31m" << (actor->name) << " : " << e.what() << "\033[0m" << std::endl;
                }
            }
        }
    }
}

std::vector<std::shared_ptr<Actor>> SceneDB::getSceneActors() {
    return scene_actors;
}

void SceneDB::changeScene() {

    std::string file_path = "resources/scenes/" + new_scene.value() + ".scene";
    
    for (auto& actor : scene_actors) {
        if (!actor->DontDestroy) {
            actor->OnDestroy();
            actor->ProcessRemovedComponents();
            actors_to_remove.emplace_back(actor);
        }
    }

    for (auto& actor : actors_to_remove) {
        scene_actors.erase(std::remove(scene_actors.begin(), scene_actors.end(), actor), scene_actors.end());
    }

    initSceneActors(file_path);
    StartScene(new_scene.value());
    new_scene.reset();
    RendererClass->renderScene(getSceneActors());
}

void SceneDB::LoadNewScene(const std::string& scene_name) {
    new_scene = scene_name;
}

bool SceneDB::proceedNewScene() {
    return new_scene.has_value();
}

void SceneDB::UpdateActors() {
    for (auto& actor : actors_to_add) {
        actor->Start(true);
        scene_actors.emplace_back(actor);
    }
    actors_to_add.clear();

    for (auto& actor : scene_actors) {
        actor->ProcessAddedComponents();
    }

    for (auto& actor : scene_actors) {
        actor->Update();
    }

    for (auto& actor : scene_actors) {
        actor->LateUpdate();
    }

    for (auto& actor : scene_actors) {
        actor->ProcessRemovedComponents();
    }

    for (auto& actor : actors_to_remove) {
        actor->OnDestroy();
        scene_actors.erase(std::remove(scene_actors.begin(), scene_actors.end(), actor), scene_actors.end());
    }
    actors_to_remove.clear();
}


luabridge::LuaRef SceneDB::Find(const std::string& name) {
    for (const auto& actor : actors_to_remove) {
        if (actor->GetName() == name) {
            return luabridge::LuaRef(ComponentManager::GetLuaState());
        }
    }
    for (const auto& actor : scene_actors) {
        if (actor->GetName() == name) {
            return luabridge::LuaRef(ComponentManager::GetLuaState(), actor.get());
        }
    }
    for (const auto& actor : actors_to_add) {
        if (actor->GetName() == name) {
            return luabridge::LuaRef(ComponentManager::GetLuaState(), actor.get());
        }
    }
    return luabridge::LuaRef(ComponentManager::GetLuaState());
}

luabridge::LuaRef SceneDB::FindAll(const std::string& name) {
    luabridge::LuaRef componentsTable = luabridge::newTable(ComponentManager::GetLuaState());
    int idx = 1;

    for (const auto& actor : scene_actors) {
        if (actor->GetName() == name) {
            componentsTable[idx++] = actor.get();
        }
    }
    for (const auto& actor : actors_to_add) {
        if (actor->GetName() == name) {
            componentsTable[idx++] = actor.get();
        }
    }
    return componentsTable;
}


luabridge::LuaRef SceneDB::Instantiate(const std::string& actor_template_name) {
    std::shared_ptr<Actor> actor;
    actor = templates->getTemplate(actor_template_name, *gameManager);
    actors_to_add.push_back(actor);
    return luabridge::LuaRef(ComponentManager::GetLuaState(), actor.get());
}

void SceneDB::Destroy(const luabridge::LuaRef& actor_ref) {
    Actor* actorPtr = actor_ref.cast<Actor*>();
    for (auto it = scene_actors.begin(); it != scene_actors.end(); ++it) {
        if (it->get() == actorPtr) {
            actorPtr->DisableAllComponents();
            actors_to_remove.push_back(*it);
            return;
        }
    }
    // Also check in actors_to_add if the actor hasn't been added to the scene yet,
    // but is scheduled to be added.
    for (auto it = actors_to_add.begin(); it != actors_to_add.end(); ++it) {
        if (it->get() == actorPtr) {
            actors_to_remove.push_back(*it);
            actors_to_add.erase(it); // Since it's scheduled to be added, we remove it from the schedule.
            return;
        }
    }
}

void SceneDB::MarkActorDontDestroyOnLoad(const luabridge::LuaRef& actor_ref) {
    Actor* actorPtr = actor_ref.cast<Actor*>();
    actorPtr->DontDestroy = true;
}


std::string SceneDB::GetCurrentScene() {
    return current_scene;
}
