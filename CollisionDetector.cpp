#include "CollisionDetector.h"

void CollisionDetector::BeginContact(b2Contact* contact)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetBody()->GetUserData().pointer);
	Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetBody()->GetUserData().pointer);

	Collision collision;
	b2Vec2 relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
	collision.relative_velocity = relative_velocity;

	if ((fixtureA->GetFilterData().categoryBits & COLLIDER) &&
		(fixtureB->GetFilterData().maskBits & COLLIDER)) {
		b2WorldManifold world_manifold;
		contact->GetWorldManifold(&world_manifold);
		collision.point = world_manifold.points[0];
		collision.normal = world_manifold.normal;
		collision.other = actorB;
		actorA->OnCollisionEnter(&collision);
		collision.other = actorA;
		actorB->OnCollisionEnter(&collision);
	}
	// Check if both are triggers based on their filter data
	else if ((fixtureA->GetFilterData().categoryBits & TRIGGER) &&
		(fixtureB->GetFilterData().maskBits & TRIGGER)) {
		collision.point = b2Vec2(-999.0f, -999.0f);
		collision.normal = b2Vec2(-999.0f, -999.0f);
		collision.other = actorB;
		actorA->OnTriggerEnter(&collision);
		collision.other = actorA;
		actorB->OnTriggerEnter(&collision);
	}
}

void CollisionDetector::EndContact(b2Contact* contact)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();
	Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetBody()->GetUserData().pointer);
	Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetBody()->GetUserData().pointer);
	Collision collision;
	b2Vec2 relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
	collision.relative_velocity = relative_velocity;


	if ((fixtureA->GetFilterData().categoryBits & COLLIDER) &&
		(fixtureB->GetFilterData().maskBits & COLLIDER)) {
		b2WorldManifold world_manifold;
		contact->GetWorldManifold(&world_manifold);
		collision.point = world_manifold.points[0];
		collision.normal = world_manifold.normal;
		collision.other = actorB;
		actorA->OnCollisionExit(&collision);
		collision.other = actorA;
		actorB->OnCollisionExit(&collision);
	}
	// Check if both are triggers based on their filter data
	else if ((fixtureA->GetFilterData().categoryBits & TRIGGER) &&
		(fixtureB->GetFilterData().maskBits & TRIGGER)) {
		collision.point = b2Vec2(-999.0f, -999.0f);
		collision.normal = b2Vec2(-999.0f, -999.0f);
		collision.other = actorB;
		actorA->OnTriggerExit(&collision);
		collision.other = actorA;
		actorB->OnTriggerExit(&collision);
	}
}