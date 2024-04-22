#ifndef SceneDB_H
#define SceneDB_H
#include "GameManager.h"
#include "ThirdParty/glm-0.9.9.8/glm/glm.hpp"
#include "ThirdParty/glm-0.9.9.8/glm/gtx/hash.hpp"
#ifdef _WIN32
    #include "ThirdParty/SDL/include/SDL.h"
#elif __APPLE__
    #include "framework/SDL2.framework/Headers/SDL.h"
#else
    #include <SDL2/SDL.h>
#endif

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include "TemplateDB.h"
#include "ComponentManager.h"
#include "Renderer.h"
#include "Actor.h"

class SceneDB
{
public:
    
	// Constructor
	SceneDB(std::string& scenefile_path,
            rapidjson::Document& renderConfig,
            Renderer* RendererClass,
            rapidjson::Document& gameConfig, AudioDB& audio,
            ComponentManager& component);    
    
    static bool proceedNewScene();
    void StartScene(const std::string& scene_name);


    static luabridge::LuaRef Find(const std::string& name);
    static luabridge::LuaRef FindAll(const std::string& name);
    static luabridge::LuaRef Instantiate(const std::string& actor_template_name);
    static void Destroy(const luabridge::LuaRef& actor_ref);

    static std::string GetCurrentScene();
    static void LoadNewScene(const std::string& scene_name);
    static void MarkActorDontDestroyOnLoad(const luabridge::LuaRef& actor_ref);

    void changeScene();

    std::vector<std::shared_ptr<Actor>> getSceneActors();
    void UpdateActors();
    static inline TemplateDataBase* templates;
    static inline GameManager* gameManager;
private:
    void initSceneActors(const std::string& scenefile_path);
    void addSceneActor(const rapidjson::Value& actorJson);
    void setActorProperties(const rapidjson::Value &actorJson, std::shared_ptr<Actor>& actor);
    SDL_Rect REGION_SIZE;
    glm::vec2 camPos;
    Renderer* RendererClass;
    rapidjson::Document& gameConfig;
    std::optional<std::string> newScene;
    AudioDB* audioDB;
    ComponentManager* componentManager;
    static inline std::vector<std::shared_ptr<Actor>> scene_actors;

    static inline std::vector<std::shared_ptr<Actor>> actors_to_add;
    static inline std::vector<std::shared_ptr<Actor>> actors_to_remove;

    static inline std::string current_scene;
    static inline std::optional<std::string> new_scene;
};
#endif
