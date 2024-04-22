#include "Raycast.h"

/// <summary>
/// 
/// </summary>
/// <param name="pos"></param>
/// <param name="dir"></param>
/// <param name="dist"> Maximum Length of the raycast </param>
/// <returns></returns>
luabridge::LuaRef Raycast::PerformRaycast(b2Vec2 pos, b2Vec2 dir, float dist)
{
    if (!Rigidbody::world || dist <= 0.f) {
        // If there's no physics world or the distance is invalid, return an empty HitResult
        return luabridge::LuaRef(ComponentManager::GetLuaState());
    }
    b2Vec2 endPos = dir;
    endPos *= dist;
    endPos += pos;
    RaycastCallback callback;
    Rigidbody::world->RayCast(&callback, pos, endPos);
    if (callback.closestFraction == 1.0f) {
        // No fixture was hit
        return luabridge::LuaRef(ComponentManager::GetLuaState());
    }
    return luabridge::LuaRef(ComponentManager::GetLuaState(), callback.hit_result);
}

luabridge::LuaRef Raycast::PerformRaycastAll(b2Vec2 pos, b2Vec2 dir, float dist)
{
    if (!Rigidbody::world || dist <= 0.f) {
        return luabridge::LuaRef(ComponentManager::GetLuaState());
    }
    b2Vec2 endPos = dir;
    endPos *= dist;
    endPos += pos;
    RayCastAllCallback callback;
    Rigidbody::world->RayCast(&callback, pos, endPos);
    std::sort(callback.hits.begin(), callback.hits.end());
    luabridge::LuaRef raycast_table = luabridge::newTable(ComponentManager::GetLuaState());
    for (size_t i = 0; i < callback.hits.size(); ++i) {
        raycast_table[i + 1] = luabridge::LuaRef(ComponentManager::GetLuaState(), callback.hits[i].hit_result);
    }
    return raycast_table;
}
