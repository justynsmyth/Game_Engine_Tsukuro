#include "RigidBody.h"

Rigidbody::Rigidbody() {
    x = 0.0f;
    y = 0.0f;
    body_type = "dynamic";
    collider_type = "box";
    trigger_type = "box";
    width = 1.0f;
    height = 1.0f;
    radius = 0.5f;
    trigger_width = 1.0f;
    trigger_height = 1.0f;
    trigger_radius = 0.5f;
    friction = 0.3f;
    bounciness = 0.3f;
    precise = true;
    gravity_scale = 1.0f;
    density = 1.0f;
    angular_friction = 0.3f;
    rotation = 0.0f;
    has_collider = true;
    has_trigger = true;
}

void Rigidbody::OnDestroy()
{
    if (body != nullptr) {
        Rigidbody::world->DestroyBody(body);
        body = nullptr;
    }
}

b2Vec2 Rigidbody::GetPosition() const {
    if (body) {
        return body->GetPosition();
    }
    return b2Vec2(x, y);
}

float Rigidbody::GetRotationRadians() const {
    float angle = body->GetAngle();
    return angle * (b2_pi / 180.0f);
}

float Rigidbody::GetRotation() const {
    float angle = body->GetAngle();
    // float radians = rotation_degrees * (b2_pi / 180.0f);
    // float degrees = rotation_radians * (180.0f / b2_pi);
    return angle * (180.0f / b2_pi);
}

void Rigidbody::Createbody() {
    b2BodyDef body_def;
    if (body_type == "dynamic") {
        body_def.type = b2_dynamicBody;
    }
    else if (body_type == "kinematic") {
        body_def.type = b2_kinematicBody;
    }
    else if (body_type == "static") {
        body_def.type = b2_staticBody;
    }

    body_def.position = b2Vec2(x, y);
    body_def.bullet = precise;
    body_def.angularDamping = angular_friction;
    body_def.angle = rotation * (b2_pi / 180.0f);;

    body_def.gravityScale = gravity_scale;
    body_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
    body = world->CreateBody(&body_def);
}

void Rigidbody::Ready()
{
    if (!world_initialized) {
        world = new b2World(b2Vec2(0.0f, 9.8f));
        collisionDetector = new CollisionDetector();
        world->SetContactListener(collisionDetector);
        world_initialized = true;
    }

    Createbody();
    if (has_collider) {
        CreateColliderFixture();
    }
    if (has_trigger) {
        CreateTriggerFixture();
    }

    if (!has_collider && !has_trigger) {
        CreateDefaultFixture();
    }
}

void Rigidbody::Cleanup() {
    if (world_initialized) {
        // Ensure all bodies and fixtures are properly destroyed before deleting the world
        b2Body* body = world->GetBodyList();
        while (body != nullptr) {
            b2Body* nextBody = body->GetNext();
            world->DestroyBody(body);
            body = nextBody;
        }

        delete collisionDetector; // Delete the collision detector instance
        collisionDetector = nullptr;

        delete world; // Delete the physics world
        world = nullptr;

        world_initialized = false;
    }
}

void Rigidbody::CreateColliderFixture()
{

    b2Shape* shape = nullptr;
    if (collider_type == "box") {
        b2PolygonShape* polygon_shape = new b2PolygonShape();
        polygon_shape->SetAsBox(width * 0.5f, height * 0.5f);
        shape = polygon_shape;
    }
    else if (collider_type == "circle") {
        b2CircleShape* circle_shape = new b2CircleShape();
        circle_shape->m_radius = radius;
        shape = circle_shape;
    }

    b2FixtureDef fixture_def;
    fixture_def.filter.categoryBits = COLLIDER;
    fixture_def.filter.maskBits = COLLIDER;
    fixture_def.shape = shape;
    fixture_def.density = density;
    fixture_def.isSensor = false;
    fixture_def.restitution = bounciness;
    fixture_def.friction = friction;
    body->CreateFixture(&fixture_def);
}

void Rigidbody::CreateTriggerFixture()
{
    b2Shape* shape = nullptr;
    if (trigger_type == "box") {
        b2PolygonShape* polygon_shape = new b2PolygonShape();
        polygon_shape->SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);
        shape = polygon_shape;
    }
    else if (trigger_type == "circle") {
        b2CircleShape* circle_shape = new b2CircleShape();
        circle_shape->m_radius = trigger_radius;
        shape = circle_shape;
    }

    b2FixtureDef fixture_def;
    fixture_def.filter.categoryBits = TRIGGER;
    fixture_def.filter.maskBits = TRIGGER;
    fixture_def.shape = shape;
    fixture_def.density = density;
    fixture_def.restitution = bounciness;
    fixture_def.friction = friction;
    fixture_def.isSensor = true;
    body->CreateFixture(&fixture_def);

}

/// <summary>
/// phantom sensor to make bodies move if neither collider nor trigger are present
/// </summary>
void Rigidbody::CreateDefaultFixture()
{
    b2PolygonShape phantom_shape;
    phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

    b2FixtureDef phantom_fixture_def;
    phantom_fixture_def.shape = &phantom_shape;
    phantom_fixture_def.density = density;

    // it is a sensor (with no callback even), no collisions will ever occur
    phantom_fixture_def.filter.categoryBits = PHANTOM;
    phantom_fixture_def.filter.maskBits = 0;
    phantom_fixture_def.isSensor = true;
    body->CreateFixture(&phantom_fixture_def);
}

void Rigidbody::AddForce(b2Vec2& vec2)
{
    body->ApplyForceToCenter(vec2, true);
}

void Rigidbody::SetVelocity(b2Vec2& vec2)
{
    body->SetLinearVelocity(vec2);
}

void Rigidbody::SetPosition(b2Vec2& vec2)
{
    if (body == nullptr) {
        x = vec2.x;
        y = vec2.y;
    }
    else {
        float curr_angle = GetRotationRadians();
        body->SetTransform(vec2, curr_angle);
    }
}

void Rigidbody::SetRotation(float degrees_clockwise)
{
    const float rotation = degrees_clockwise * (b2_pi / 180.0f);
    b2Vec2 curr_pos = body->GetPosition();
    body->SetTransform(curr_pos, rotation);
}

void Rigidbody::SetAngularVelocity(float degrees_clockwise)
{
    // need to convert degrees to radianns
    // then reverse clockwise rotation to counterclockwise
    const float radians_per_sec = degrees_clockwise * (b2_pi / 180.0f);
    body->SetAngularVelocity((radians_per_sec));
}

void Rigidbody::SetGravityScale(float gravity_scale)
{
    body->SetGravityScale(gravity_scale);
}

void Rigidbody::SetUpDirection(b2Vec2& direction)
{
    direction.Normalize();
    b2Vec2 curr_pos = body->GetPosition();
    float new_angle_radians = glm::atan(direction.x, -direction.y);
    body->SetTransform(curr_pos, new_angle_radians);
}

void Rigidbody::SetRightDirection(b2Vec2& direction)
{
    direction.Normalize();
    b2Vec2 curr_pos = body->GetPosition();
    float new_angle_radians = glm::atan(direction.x, -direction.y) - b2_pi / 2.0f;
    body->SetTransform(curr_pos, new_angle_radians);
}

b2Vec2 Rigidbody::GetVelocity()
{
    return body->GetLinearVelocity();
}

float Rigidbody::GetAngularVelocity()
{
    return body->GetAngularVelocity() * (180.0f / b2_pi);
}

float Rigidbody::GetGravityScale()
{
    return body->GetGravityScale();
}

b2Vec2 Rigidbody::GetUpDirection()
{
    float angle = body->GetAngle();

    // Up vector according to screen coordinates (0, -1) when angle is zero.
    // Remember, Box2D uses radians and +angle counterclockwise, but screen "up" is -y.
    return b2Vec2(glm::sin(angle), -glm::cos(angle));
}

b2Vec2 Rigidbody::GetRightDirection()
{
    // in radians
    float angle = body->GetAngle();
    // Right vector is in the direction of positive x-axis (1, 0) when angle is zero.
    return b2Vec2(glm::cos(angle), glm::sin(angle));
}

void Rigidbody::PhysicsStep()
{
    if (world_initialized) {
        world->Step(1.0f / 60.0f, 8, 3);
    }
}
