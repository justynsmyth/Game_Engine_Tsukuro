#pragma once
#include <string>

#include "box2d/box2d.h"
#include <iostream>
#include "CollisionDetector.h"
#include "ThirdParty/glm-0.9.9.8/glm/glm.hpp"
class Actor;

class Rigidbody
{
public:
	Rigidbody();
	void OnDestroy();
	bool enabled;
	std::string key;
	std::string type;
	std::string body_type;
	std::string collider_type;
	std::string trigger_type;

	void copyParameters(const Rigidbody& other) {
		this->x = other.x;
		this->y = other.y;
		this->width = other.width;
		this->height = other.height;
		this->radius = other.radius;
		this->trigger_width = other.trigger_width;
		this->trigger_height = other.trigger_height;
		this->trigger_radius = other.trigger_radius;
		this->friction = other.friction;
		this->bounciness = other.bounciness;
		this->precise = other.precise;
		this->gravity_scale = other.gravity_scale;
		this->density = other.density;
		this->angular_friction = other.angular_friction;
		this->rotation = other.rotation;
		this->has_collider = other.has_collider;
		this->has_trigger = other.has_trigger;
		this->enabled = other.enabled;
		this->key = other.key;
		this->type = other.type;
		this->body_type = other.body_type;
		this->collider_type = other.collider_type;
		this->trigger_type = other.trigger_type;
	}

	Actor* actor = nullptr;

	float x;
	float y;
	float width;
	float height;
	float radius;
	float trigger_width;
	float trigger_height;
	float trigger_radius;
	float friction;
	float bounciness;
	bool precise;
	float gravity_scale;
	float density;
	float angular_friction;
	float rotation;
	bool has_collider;
	bool has_trigger;

	b2Vec2 GetPosition() const;

	float GetRotationRadians() const;
	float GetRotation() const;
	void Createbody();
	void Cleanup();
	void Ready();
	void CreateColliderFixture();
	void CreateTriggerFixture();
	void CreateDefaultFixture();

	void AddForce(b2Vec2& vec2);
	void SetVelocity(b2Vec2& vec2);
	void SetPosition(b2Vec2& vec2);
	void SetRotation(float degrees_clockwise);
	void SetAngularVelocity(float degrees_clockwise);
	void SetGravityScale(float gravity_scale);
	void SetUpDirection(b2Vec2& direction);
	void SetRightDirection(b2Vec2& direction);

	b2Vec2 GetVelocity();
	float GetAngularVelocity();
	float GetGravityScale();
	b2Vec2 GetUpDirection();
	b2Vec2 GetRightDirection();

	static inline bool world_initialized = false;
	static inline b2World* world;
	static inline CollisionDetector* collisionDetector;
	static void PhysicsStep();
private:
	b2Body* body;
};
