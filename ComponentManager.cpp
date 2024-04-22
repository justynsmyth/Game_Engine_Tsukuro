#include "ComponentManager.h"
#include <fstream>
#include <filesystem>
#include "Actor.h"
#include "SceneDB.h"


namespace fs = std::filesystem;

ComponentManager::ComponentManager() {
	lua_state = nullptr;
}

void ComponentManager::Initialize() {
	InitializeState();
	InitializeFunctions();
	InitializeComponents();
}

void ComponentManager::InitializeState() {
	lua_state = luaL_newstate();
	luaL_openlibs(lua_state);
}

/*
 * Where C++ functions are exposed to the Lua scripting environment.
 * This is done using Luabridge to create bindings that allow Lua scripts to call C++ functions.
 */
void ComponentManager::InitializeFunctions() {

	luabridge::getGlobalNamespace(lua_state)
		.beginNamespace("Debug")
		    .addFunction("Log", ComponentManager::Print)
		    .addFunction("LogError", ComponentManager::PrintError)
		.endNamespace();
    /* Actor class instance */
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Actor>("Actor")
            .addFunction("GetName", &Actor::GetName)
            .addFunction("GetID", &Actor::GetID)
            .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
            .addFunction("GetComponent", &Actor::GetComponent)
            .addFunction("GetComponents", &Actor::GetComponents)
            .addFunction("AddComponent", &Actor::LuaAddComponent)
            .addFunction("RemoveComponent", &Actor::LuaRemoveComponent)
        .endClass();
    /* glm::vec instances */
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<glm::vec2>("vec2")
            .addData("x", &glm::vec2::x)
            .addData("y", &glm::vec2::y)
        .endClass();
    /* Actor namespace instance */
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Actor")
            .addFunction("Find", &SceneDB::Find)
            .addFunction("FindAll", &SceneDB::FindAll)
            .addFunction("Instantiate", &SceneDB::Instantiate)
            .addFunction("Destroy", &SceneDB::Destroy)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Application")
            .addFunction("GetFrame", &Helper::GetFrameNumber)
            .addFunction("Quit", ComponentManager::LuaQuit)
            .addFunction("Sleep", ComponentManager::LuaSleep)
            .addFunction("OpenURL", ComponentManager::LuaOpenUrl)
        .endNamespace();
    /* Audio instance*/
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Audio")
            .addFunction("Play", AudioDB::PlayChannel)
            .addFunction("Halt", AudioDB::HaltChannel)
            .addFunction("SetVolume", AudioDB::SetVolume)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Image")
        .addFunction("DrawUI", Renderer::LuaDrawUI)
        .addFunction("DrawUIEx", Renderer::LuaDrawUIEx)
        .addFunction("Draw", Renderer::LuaDraw)
        .addFunction("DrawEx", Renderer::LuaDrawEx)
        .addFunction("DrawPixel", Renderer::LuaDrawPixel)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Text")
            .addFunction("Draw", Renderer::LuaDrawText)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Input")
            .addFunction("GetKey", Input::GetKey)
            .addFunction("GetKeyDown", Input::GetKeyDown)
            .addFunction("GetKeyUp", Input::GetKeyUp)
            .addFunction("GetMousePosition", Input::GetMousePosition)
            .addFunction("GetMouseButton", Input::GetMouseButton)
            .addFunction("GetMouseButtonDown", Input::GetMouseButtonDown)
            .addFunction("GetMouseButtonUp", Input::GetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", Input::GetMouseScrollDelta)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Camera")
        .addFunction("SetPosition", Renderer::SetCameraPosition)
        .addFunction("GetPositionX", Renderer::GetCameraPositionX)
        .addFunction("GetPositionY", Renderer::GetCameraPositionY)
        .addFunction("SetZoom", Renderer::SetZoomFactor)
        .addFunction("GetZoom", Renderer::GetZoomFactor)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Scene")
        .addFunction("Load", SceneDB::LoadNewScene)
        .addFunction("GetCurrent", SceneDB::GetCurrentScene)
        .addFunction("DontDestroy", SceneDB::MarkActorDontDestroyOnLoad)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<b2Vec2>("Vector2")
        .addConstructor<void(*) (float, float)>()
        .addData("x", &b2Vec2::x)
        .addData("y", &b2Vec2::y)
        .addFunction("Normalize", &b2Vec2::Normalize)
        .addFunction("Length", &b2Vec2::Length)
        .addFunction("__add", &b2Vec2::operator_add)
        .addFunction("__sub", &b2Vec2::operator_sub)
        .addFunction("__mul", &b2Vec2::operator_mul)
        .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
        .addStaticFunction("Distance", &b2Distance)
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<b2Vec3>("Vector3")
        .addConstructor<void(*) (float, float, float)>()
        .addData("x", &b2Vec3::x)
        .addData("y", &b2Vec3::y)
        .addData("z", &b2Vec3::z)
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Rigidbody>("Rigidbody")
        .addConstructor<void (*) (void)>()
        .addData("x", &Rigidbody::x)
        .addData("y", &Rigidbody::y)
        .addData("body_type", &Rigidbody::body_type)
        .addFunction("Ready", &Rigidbody::Ready)
        .addData("enabled", &Rigidbody::enabled)
        .addData("key", &Rigidbody::key)
        .addData("type", &Rigidbody::type)
        .addData("actor", &Rigidbody::actor)
        .addData("gravity_scale", &Rigidbody::gravity_scale)
        .addData("precise", &Rigidbody::precise)
        .addData("density", &Rigidbody::density)
        .addData("angular_friction", &Rigidbody::angular_friction)
        .addData("rotation", &Rigidbody::rotation)
        .addData("has_collider", &Rigidbody::has_collider)
        .addData("has_trigger", &Rigidbody::has_trigger)
        .addData("collider_type", &Rigidbody::collider_type)
        .addData("trigger_type", &Rigidbody::trigger_type)
        .addData("width", &Rigidbody::width)
        .addData("height", &Rigidbody::height)
        .addData("radius", &Rigidbody::radius)
        .addData("trigger_width", &Rigidbody::trigger_width)
        .addData("trigger_height", &Rigidbody::trigger_height)
        .addData("trigger_radius", &Rigidbody::trigger_radius)
        .addData("friction", &Rigidbody::friction)
        .addData("bounciness", &Rigidbody::bounciness)
        .addFunction("GetPosition", &Rigidbody::GetPosition)
        .addFunction("GetRotation", &Rigidbody::GetRotation)
        .addFunction("AddForce", &Rigidbody::AddForce)
        .addFunction("SetVelocity", &Rigidbody::SetVelocity)
        .addFunction("SetRotation", &Rigidbody::SetRotation)
        .addFunction("SetPosition", &Rigidbody::SetPosition)
        .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
        .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
        .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
        .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
        .addFunction("GetVelocity", &Rigidbody::GetVelocity)
        .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
        .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
        .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
        .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Collision>("collision")
        .addConstructor<void (*) (void)>()
        .addData("other", &Collision::other)
        .addData("point", &Collision::point)
        .addData("relative_velocity", &Collision::relative_velocity)
        .addData("normal", &Collision::normal)
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Physics")
        .addFunction("Raycast", &Raycast::PerformRaycast)
        .addFunction("RaycastAll", &Raycast::PerformRaycastAll)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<HitResult>("HitResult")
        .addConstructor<void (*) (Actor*, const b2Vec2&, const b2Vec2&, bool)>()
        .addData("actor", &HitResult::actor)
        .addData("point", &HitResult::point)
        .addData("normal", &HitResult::normal)
        .addData("is_trigger", &HitResult::is_trigger)
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Event")
        .addFunction("Publish", &EventBus::Publish)
        .addFunction("Subscribe", &EventBus::Subscribe)
        .addFunction("Unsubscribe", &EventBus::Unsubscribe)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("VRSystem")
        .addFunction("Initialize", &openvr_utils::Initialize)
        .addFunction("GetDeviceTranslation", &openvr_utils::GetDevicePosition)
        .addFunction("GetDeviceForwardVector", &openvr_utils::GetDeviceForwardVector)
        .addFunction("GetJoystickPosition", &openvr_utils::GetJoystickPosition)
        .addFunction("GetDevicePose", &openvr_utils::GetDevicePosition)
        .addFunction("TriggerHaptic", &openvr_utils::TriggerHaptic)
        .addFunction("GetKey", &VRInput::GetKey)
        .addFunction("GetKeyDown", &VRInput::GetKeyDown)
        .addFunction("GetKeyUp", &VRInput::GetKeyUp)
        .endNamespace();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Position>("Position")
        .addData("x", &Position::x)
        .addData("y", &Position::y)
        .addData("z", &Position::z)
        .addConstructor<void (*) (void)>()
        .endClass();
    luabridge::getGlobalNamespace(lua_state)
        .beginClass<Quaternion>("Quaternion")
        .addData("w", &Quaternion::w)
        .addData("x", &Quaternion::x)
        .addData("y", &Quaternion::y)
        .addData("z", &Quaternion::z)
        .addConstructor<void (*) (void)>()
        .endClass();

}


/*
 * Handles the loading of Lua script files that define the components
 * for game entities. These scripts are like templates that spell out the behaviors and properties
 * a component will have when instantiated. By loading these Lua files, we ensure that the component
 * definitions are available in the Lua state and ready to be associated with game objects (actors).
 * This involves iterating through all the .lua files in a specific directory, loading each one, and
 * potentially storing a reference to each component's Lua table or function in a way that they can
 * be easily instantiated and attached to actors when needed.
 */
void ComponentManager::InitializeComponents() {
    const std::string componentDir = "resources/component_types/";
    if (!fs::exists(componentDir)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(componentDir)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == ".lua") {
            if (luaL_dofile(lua_state, entry.path().string().c_str()) != LUA_OK) {
                std::string errorMessage = lua_tostring(lua_state, -1);  // Get error message from stack
                std::cout << "Problem with lua file: " << entry.path().string() << "\n";
                std::cout << "Lua error message: " << errorMessage << std::endl;  // Display Lua error
                lua_pop(lua_state, 1);  // Pop error message from stack
                exit(0);
            }
            else {
                std::string componentName = entry.path().stem().string();

                component_tables.insert({ componentName,
                    std::make_shared<luabridge::LuaRef>(
                        luabridge::getGlobal(lua_state, componentName.c_str()))
                    });
            }
        }
    }
}

luabridge::LuaRef ComponentManager::CreateRigidbody()
{
    Rigidbody* rigidbody = new Rigidbody();
    luabridge::LuaRef componentRef(lua_state, rigidbody);

    std::string componentName = "Rigidbody";
    if (component_tables.find(componentName) == component_tables.end()) {
        component_tables.insert({ componentName,
                    std::make_shared<luabridge::LuaRef>(componentRef)
            });
    }
    return componentRef;
}


bool ComponentManager::find_component(std::string& component_name) const {
    return component_tables.find(component_name) != component_tables.end();
}


void ComponentManager::EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table) {
    luabridge::LuaRef metatable = luabridge::newTable(lua_state);
    metatable["__index"] = parent_table;

    instance_table.push(lua_state);
    metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1); // Pop the instance_table off the stack
}

luabridge::LuaRef ComponentManager::CreateComponentInstance(const std::string& component_type_name) {
    auto it = component_tables.find(component_type_name);
    if (it == component_tables.end()) {
        std::cerr << "Component type '" << component_type_name << "' not loaded." << std::endl;
        return luabridge::LuaRef(lua_state);
    }

    // Get the global base table for the component type (from component_tables).
    luabridge::LuaRef base_table_ref = *(it->second);

    // Create a new table in Lua for the new component instance.
    luabridge::LuaRef instance_table = luabridge::newTable(lua_state);

    EstablishInheritance(instance_table, base_table_ref);

    return instance_table;
}

luabridge::LuaRef ComponentManager::CloneComponentInstance(luabridge::LuaRef& componentTemplate) {
    // Create a new table in Lua for the new component instance.
    luabridge::LuaRef instance_table = luabridge::newTable(lua_state);

    EstablishInheritance(instance_table, componentTemplate);

    // The instance now has access to all methods and properties defined in the base table.
    // Wrap the new instance table in a shared_ptr to manage its lifetime.
    return instance_table;
}