#pragma once

#include "box2d/box2d.h"
#include <iostream>
#include <memory>
#include "Actor.h"
/*
Things we can do:

can be an argument to a function
pointer of a forward declared class can be a member variable (NOT REFERENCE OR DIRECT)
*/

class CollisionDetector : public b2ContactListener
{
	void BeginContact(b2Contact* contact);
	void EndContact(b2Contact* contact);

	/*
	OnTriggerEnter: Called when a trigger overlaps with another trigger
	OnTriggerExit: Called when a trigger stops overlapping with another trigger
	OnCollisionEnter: Called when a collider overlaps with another collider
	OnCollisionExit: Called when a collider stops overlapping with another collider
	*/
};

class Collision {
public:
	Actor* other;
	b2Vec2 point;
	b2Vec2 relative_velocity;
	b2Vec2 normal;
};  

enum FixtureCategory {
	COLLIDER = 0x0001,
	TRIGGER = 0x0002,
	PHANTOM = 0x0004
};