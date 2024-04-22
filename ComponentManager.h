#ifndef ComponentManager_H
#define ComponentManager_H
class Actor; // Forward declaration

#include <iostream>
#include "ThirdParty/Lua/src/lua.hpp"
#include "ThirdParty/LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include "Input.h"
#include "Renderer.h"
#include "RigidBody.h"
#include "Raycast.h"
#include "EventBus.h"
#include <thread>
#include "openvr_utils.h"
class Collision;
class HitResult;

class ComponentManager
{
public:
	ComponentManager();
	void Initialize();

	bool find_component(std::string& component_name) const;

	static void LuaSleep(int ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	static void LuaQuit() {
		exit(0);
	}

	static void LuaOpenUrl(const std::string& url) {
#ifdef _WIN32
		std::string cmd = "start " + url;
#elif __APPLE__
		std::string cmd = "open " + url;
#else
		std::string cmd = "xdg-open " + url;
#endif
		std::system(cmd.c_str());
	}

	static void Print(const std::string& message) {
		std::cout << message << std::endl;
	}

	static void PrintError(const std::string& message) {
		std::cerr << message << std::endl;
	}
	static luabridge::LuaRef CreateComponentInstance(const std::string& component_type_name);
	luabridge::LuaRef CloneComponentInstance(luabridge::LuaRef& componentTemplate);
	static lua_State* GetLuaState() {
		return lua_state;
	}
	static luabridge::LuaRef CreateRigidbody();


private:
	static void EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table);
	void InitializeState();
	void InitializeFunctions();
	void InitializeComponents();
	inline static std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> component_tables;
	inline static lua_State* lua_state;
};
#endif
