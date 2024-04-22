#pragma once
#include "ThirdParty/Lua/src/lua.hpp"
#include "ThirdParty/LuaBridge/LuaBridge.h"
#include "box2d/box2d.h"
#include <iostream>
#include <memory>
#include "RigidBody.h"
#include "ComponentManager.h"
class Actor;
class HitResult;

class Raycast
{
public:
	static luabridge::LuaRef PerformRaycast(b2Vec2 pos, b2Vec2 dir, float dist);
    static luabridge::LuaRef PerformRaycastAll(b2Vec2 pos, b2Vec2 dir, float dist);
};

class HitResult
{
public:
	HitResult(Actor* actor, const b2Vec2& point, const b2Vec2& normal, bool is_trigger)
		: actor(actor), point(point), normal(normal), is_trigger(is_trigger) {}
	Actor* actor;
	b2Vec2 point;
	b2Vec2 normal;
	bool is_trigger;
};

class RaycastCallback : public b2RayCastCallback {
public:
	HitResult hit_result;
    float closestFraction = 1.0f;  // Start with the end of the raycast

    RaycastCallback() : hit_result(nullptr, { 0, 0 }, { 0, 0 }, false) {}

    /// <summary>
    /// The goal is find a raycast that is the closest hit encountered along the path. It will update closestFraction until it finds a value.
    /// </summary>
    /// <param name="fixture"> The fixture that the raycast hit. </param>
    /// <param name="point"> The point at which the raycast hits the fixture. </param>
    /// <param name="normal"> The normal vector at the point of impact, useful for determining the angle of reflection. </param>
    /// <param name="fraction"> The fraction along the ray's path where the hit occurred. This is a value between 0.0 and 1.0, where 0.0 means the start of the ray, and 1.0 means the end point specified for the raycast.</param>
    /// <returns></returns>
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetBody()->GetUserData().pointer);
        if (actor == nullptr) {
            // Phantom fixture detected. Ignore it.
            return -1.0f;
        }
        if (fixture->GetFilterData().categoryBits == PHANTOM) {
            return -1.0f; // Ignore this fixture
        }
        // Check if this is the closest hit and that it's not starting inside a fixture
        if (fraction < closestFraction && fraction > 0.0f) {
            closestFraction = fraction;  // Update the closest fraction
            hit_result = HitResult(reinterpret_cast<Actor*>(fixture->GetBody()->GetUserData().pointer), point, normal, fixture->IsSensor());
        }
        // Return the fraction to continue the raycast
        return fraction;
    }
};

class RayCastAllCallback : public b2RayCastCallback {
public:
    struct SortedHit {
        HitResult hit_result;
        float fraction;

        SortedHit(const HitResult& hit, float frac)
            : hit_result(hit), fraction(frac) {}

        bool operator<(const SortedHit& other) const {
            return fraction < other.fraction;
        }
    };

    std::vector<SortedHit> hits;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
        const b2Vec2& normal, float fraction) override {
        if (fixture->GetFilterData().categoryBits == PHANTOM) {
            return -1.0f;
        }
        // Ignore the hit if it starts inside a fixture
        if (fraction <= 0) {
            return -1.0f;
        }

        Actor* actor = reinterpret_cast<Actor*>(fixture->GetBody()->GetUserData().pointer);
        // Phantom fixture detected. Ignore it.
        if (actor == nullptr) {
            return -1.0f;
        }


        HitResult hit(actor, point, normal, fixture->IsSensor());
        hits.emplace_back(hit, fraction);

        // Return the fraction to continue the raycast
        return 1;
    }
};
