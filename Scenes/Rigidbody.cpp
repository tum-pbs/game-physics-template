#include "Rigidbody.h"
#include "glm/gtx/string_cast.hpp"

using vec3 = glm::vec3;
using mat4 = glm::mat4;
using quat = glm::quat;
using vec4 = glm::vec4;

Rigidbody::Rigidbody(vec3 &position, quat &rotation, vec3 &scale, float mass)
    : position(position),
      rotation(rotation),
      scale(scale),
      mass(mass),
      velocity(vec3(0)),
      angularMomentum(vec3(0)),
      frameForce(vec3(0)),
      frameTorque(vec3(0))
{
    inverseInertiaTensor = glm::inverse(getInertiaTensor());
}

Rigidbody::Rigidbody(vec3 &position, vec3 &scale, float mass) : Rigidbody(position, quat(1, 0, 0, 0), scale, mass) {};

CollisionInfo Rigidbody::collide(Rigidbody &body0, Rigidbody &body1)
{
    mat4 worldFromObj_0 = body0.getWorldFromObj();
    mat4 worldFromObj_1 = body1.getWorldFromObj();
    CollisionInfo info = collisionTools::checkCollisionSAT(worldFromObj_0, worldFromObj_1);
    if (!info.isColliding)
        return info;

    vec3 objX_A = info.collisionPointWorld - body0.position;
    vec3 objX_b = info.collisionPointWorld - body1.position;

    mat4 rotation0 = glm::toMat4(body0.rotation);
    mat4 rotation1 = glm::toMat4(body1.rotation);
    mat4 currentInertiaTensorInverse0 = rotation0 * body0.inverseInertiaTensor * glm::transpose(rotation0);
    mat4 currentInertiaTensorInverse1 = rotation1 * body1.inverseInertiaTensor * glm::transpose(rotation1);
    vec3 angularVel_A = currentInertiaTensorInverse0 * vec4(body0.angularMomentum, 0);
    vec3 angularVel_B = currentInertiaTensorInverse1 * vec4(body1.angularMomentum, 0);

    vec3 velocityA = body0.velocity + glm::cross(angularVel_A, objX_A);
    vec3 velocityB = body1.velocity + glm::cross(angularVel_B, objX_b);

    float relVelonNormal = glm::dot(velocityA - velocityB, info.normalWorld);
    if (relVelonNormal > 0.0f)
        return info; // leaving each other, collide before

    const float elasticity = 1.0f; // todo: set as a user input param
    const float numerator = -(1.0f + elasticity) * relVelonNormal;
    const float inverseMasses = 1 / body0.mass + 1 / body1.mass;

    vec3 rma = cross(vec3(currentInertiaTensorInverse0 * vec4(cross(objX_A, info.normalWorld), 0)), objX_A);
    vec3 rmb = cross(vec3(currentInertiaTensorInverse1 * vec4(cross(objX_b, info.normalWorld), 0)), objX_b);
    const float rmab = dot(rma + rmb, info.normalWorld);
    const float denominator = inverseMasses + rmab;

    const float impulse = numerator / denominator;

    vec3 impulseNormal = impulse * info.normalWorld;
    body0.velocity += impulseNormal / body0.mass;
    body1.velocity -= impulseNormal / body1.mass;

    body0.angularMomentum += cross(objX_A, impulseNormal);
    body1.angularMomentum -= cross(objX_b, impulseNormal);

    return info;
}

void Rigidbody::addLocalForce(vec3 &force, vec3 &where)
{
    frameForce += force;
    frameTorque += glm::cross(where, force);
}

void Rigidbody::addWorldForce(vec3 &force, vec3 &where)
{
    addLocalForce(force, where - position);
}

void Rigidbody::update(float deltaTime)
{
    // x_cm <- x_cm + h * v_cm
    position += deltaTime * velocity;

    // v_cm <- v_cm + h * F / m
    velocity += deltaTime * frameForce / mass;

    // L <- L + h * q
    angularMomentum += deltaTime * frameTorque;

    // I^-1 = Rot_r * I_0^-1 * Rot_r^T
    mat4 rotationMatrix = glm::toMat4(rotation);
    mat4 currentInverseInertiaTensor = rotationMatrix * inverseInertiaTensor * glm::transpose(rotationMatrix);

    // w <- I^-1 * L
    // vec4 with 0 in the last component to ignore translation, which should be 0
    angularVelocity = currentInverseInertiaTensor * vec4(angularMomentum, 0);

    // r <- r + h/2 * (0,w)^T * r
    quat angularDisplacement = quat(0, angularVelocity.x, angularVelocity.y, angularVelocity.z);
    rotation += deltaTime / 2.0f * angularDisplacement * rotation;
    rotation = glm::normalize(rotation);

    frameForce = vec3(0);
    frameTorque = vec3(0);
}

mat4 Rigidbody::getWorldFromObj()
{
    mat4 rotationMatrix = glm::toMat4(rotation);
    mat4 scaleMatrix = glm::scale(mat4(1), scale);
    mat4 translationMatrix = glm::translate(mat4(1), position);
    return translationMatrix * rotationMatrix * scaleMatrix;
}

mat4 Rigidbody::getInertiaTensor()
{
    // assumption: homogenous cuboid
    const float xSquared = scale.x * scale.x;
    const float ySquared = scale.y * scale.y;
    const float zSquared = scale.z * scale.z;

    return mat4(
        1 / 12.0f * mass * (ySquared + zSquared), 0.0f, 0.0f, 0.0f,
        0.0f, 1 / 12.0f * mass * (xSquared + zSquared), 0.0f, 0.0f,
        0.0f, 0.0f, 1 / 12.0f * mass * (xSquared + ySquared), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}