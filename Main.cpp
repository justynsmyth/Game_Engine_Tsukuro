#include <iostream>
#include <filesystem>
#include <cstdlib>
#include "GameManager.h"
#include "SceneDB.h"
#include "ThirdParty/rapidjson/include/rapidjson/document.h"
#include "TemplateDB.h"
#include "Renderer.h"
#include "ComponentManager.h"
#include "AudioDB.h"
#include "Input.h"
#include "ThirdParty/Lua/src/lua.hpp"
#include "ThirdParty/LuaBridge/LuaBridge.h"
#include "RigidBody.h"
#include "EventBus.h"
#include "openvr_utils.h"
#include "openvr-master/headers/openvr.h"
namespace fs = std::filesystem;

// Scale -> Rotate --> Transform
void LoadInitialSettings(GameManager& game, rapidjson::Document& renderingConfig, rapidjson::Document& gameConfig, std::string & scene);
void InitComponents(ComponentManager & c);
void checkResources();
void initGameConfig(rapidjson::Document& doc);


int main(int argc, char*argv[]) {
    // initialize Game State
    GameManager game(3, 0);
    rapidjson::Document renderingConfig;
    rapidjson::Document gameConfig;
    std::string scene_path;

    LoadInitialSettings(game, renderingConfig, gameConfig, scene_path);

    ComponentManager components;
    InitComponents(components);

    
    // initialize template class
    TemplateDataBase templates(components);
    AudioDB audio;

    TTF_Init();
    Input::Init();

    openvr_utils Open_VR;

    Renderer rendererClass(renderingConfig, gameConfig, &audio, &game, &Open_VR);

    SDL_Renderer* renderer = rendererClass.getRenderer();

    SceneDB::gameManager = &game;
    SceneDB::templates = &templates;

    SceneDB sceneDB(scene_path, renderingConfig, &rendererClass, gameConfig, audio, components);

    std::string scene_name = gameConfig["initial_scene"].GetString();
    sceneDB.StartScene(scene_name);

	while (game.running()) {
        rendererClass.ProcessInput();
        bool vr_mode = Open_VR.VRActivated();
        if (vr_mode) {
            Open_VR.HandleInput();
        }
        SDL_RenderClear(renderer);
        sceneDB.UpdateActors();
        EventBus::ProcessSubscriptions();
        Rigidbody::PhysicsStep();
        // For SDL
        if (!rendererClass.isOpenGL()) {
            rendererClass.renderScene(sceneDB.getSceneActors());
        }
        else {
            Open_VR.RenderFrame();
        }
        if (sceneDB.proceedNewScene()) {
            sceneDB.changeScene();
        }
        Input::LateUpdate();
        if (vr_mode) {
            VRInput::LateUpdate();
        }
        // Need to do following line because we wrote to a buffer, but it won't display changes
        // UNLESS we change active_buffer to new frame. This is done to avoid screen tearing
        // We essentially flip between buffers, clear the unused buffer before flipping BACK to display a new frame
        if (!rendererClass.isOpenGL()) {
            Helper::SDL_RenderPresent498(renderer);
        }
	}
    vr::VR_Shutdown();
	return 0;
}

void InitComponents(ComponentManager& c) {
    c.Initialize();
}

/*
 Check for /resources folder in root directory
 REQUIRES: /resources/game.config file as well
 */
void checkResources() {
	std::string path = "resources";
	if (!fs::exists(path)) {
		std::cout << "error: resources/ missing";
		exit(0);
	}
	path += "/game.config";
	if (!fs::exists(path)) {
		std::cout << "error: resources/game.config missing";
		exit(0);
	}
}

void LoadInitialSettings(GameManager & game, rapidjson::Document & renderingConfig, rapidjson::Document & gameConfig, std::string& scene) {
    // check for Resources folder and game.config file
    checkResources();
    game.ReadJsonFile("resources/rendering.config", renderingConfig);
    game.ReadJsonFile("resources/game.config", gameConfig);
    initGameConfig(gameConfig);
    std::stringstream init_scene;
    init_scene << "resources/scenes/" << gameConfig["initial_scene"].GetString()
        << ".scene";
    scene = init_scene.str();
}

void initGameConfig(rapidjson::Document& doc) {
    // check initial_scene exists in game.config
    if (!doc.HasMember("initial_scene")) {
        std::cout << "error: initial_scene unspecified";
        exit(0);
    }
    // check for file for initial_scene in /resources/scenes/ folder
    std::stringstream ss;
    ss << "resources/scenes";
    if (!fs::exists(ss.str())) {
        std::cout << "error: resources/scenes missing";
        exit(0);
    }
    ss << "/" << doc["initial_scene"].GetString() << ".scene";
    if (!fs::exists(ss.str())) {
        std::cout << "error: scene " << doc["initial_scene"].GetString()
                  << " is missing";
        exit(0);
    }

}
