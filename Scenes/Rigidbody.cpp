#include "Rigidbody.h"
#include "glm/gtx/string_cast.hpp"

using vec3 = glm::vec3;
using mat4 = glm::mat4;
using quat = glm::quat;
using vec4 = glm::vec4;

bool Rigidbody::collide(Rigidbody &body0, Rigidbody &body1)
{
    CollisionInfo info = collisionTools::checkCollisionSAT(body0.worldFromObj, body1.worldFromObj); //, body0.m_scale, body1.m_scale
    if (!info.isValid)
        return false;
    body0.collisonPoint = info.collisionPointWorld;
    body1.collisonPoint = body0.collisonPoint;
    body0.collisioNormal = info.normalWorld;
    body1.collisioNormal = -info.normalWorld;
    vec3 xaWorld = info.collisionPointWorld - body0.position;
    vec3 xbWorld = info.collisionPointWorld - body1.position;

    // these obj positions are just used to print some message in cmd. Only for debug, not used in the impulse calculation
    vec3 xa_objA = body0.objFromWorld * vec4(info.collisionPointWorld, 1);
    vec3 xb_objB = body1.objFromWorld * vec4(info.collisionPointWorld, 1);
    // end obj positions

    mat4 rotation0 = glm::toMat4(body0.rotation);
    mat4 rotation1 = glm::toMat4(body1.rotation);
    mat4 rotationTranspose0 = glm::transpose(rotation0);
    mat4 rotationTranspose1 = glm::transpose(rotation1);
    mat4 currentInertiaTensorInverse0 = rotation0 * body0.inverseInertiaTensor * rotationTranspose0;
    mat4 currentInertiaTensorInverse1 = rotation1 * body1.inverseInertiaTensor * rotationTranspose1;
    vec3 angularVel_A = currentInertiaTensorInverse0 * vec4(body0.angularMomentum, 0);
    vec3 angularVel_B = currentInertiaTensorInverse1 * vec4(body1.angularMomentum, 0);

    vec3 velocityA = body0.velocity + glm::cross(angularVel_A, xaWorld);
    vec3 velocityB = body1.velocity + glm::cross(angularVel_B, xbWorld);

    body0.relVelocity = velocityA - velocityB;
    body0.totalVelocity = velocityA;
    body1.totalVelocity = velocityB;
    float relVelonNormal = glm::dot(velocityA - velocityB, info.normalWorld);
    if (relVelonNormal > 0.0f)
        return false; // leaving each other, collide before

    const float elasticity = 1.0f; // todo: set as a user input param
    const float numerator = -(1.0f + elasticity) * relVelonNormal;
    const float inverseMasses = 1 / body0.mass + 1 / body1.mass;

    vec3 rma = cross(vec3(currentInertiaTensorInverse0 * vec4(cross(xaWorld, info.normalWorld), 0)), xaWorld);
    vec3 rmb = cross(vec3(currentInertiaTensorInverse1 * vec4(cross(xbWorld, info.normalWorld), 0)), xbWorld);
    const float rmab = dot(rma + rmb, info.normalWorld);
    const float denominator = inverseMasses + rmab;

    const float impulse = numerator / denominator;

    vec3 impulseNormal = impulse * info.normalWorld;
    body0.velocity += impulseNormal / body0.mass;
    body1.velocity -= impulseNormal / body1.mass;

    body0.angularMomentum += cross(xaWorld, impulseNormal);
    body1.angularMomentum -= cross(xbWorld, impulseNormal);

    return true;
}

std::vector<vec3> Rigidbody::getCorners()
{
    const vec3 worldCenter = worldFromObj * vec4(0, 0, 0, 1);
    vec3 worldEdges[3];
    for (size_t i = 0; i < 3; ++i)
    {
        vec3 objEdge = vec3(0.0);
        objEdge[i] = 0.5f;
        worldEdges[i] = worldFromObj * vec4(objEdge, 0);
    }
    std::vector<vec3> results;
    results.push_back(worldCenter - worldEdges[0] - worldEdges[1] - worldEdges[2]);
    results.push_back(worldCenter + worldEdges[0] - worldEdges[1] - worldEdges[2]);
    results.push_back(worldCenter - worldEdges[0] + worldEdges[1] - worldEdges[2]);
    results.push_back(worldCenter + worldEdges[0] + worldEdges[1] - worldEdges[2]); // this +,+,-
    results.push_back(worldCenter - worldEdges[0] - worldEdges[1] + worldEdges[2]);
    results.push_back(worldCenter + worldEdges[0] - worldEdges[1] + worldEdges[2]); // this +,-,+
    results.push_back(worldCenter - worldEdges[0] + worldEdges[1] + worldEdges[2]); // this -,+,+
    results.push_back(worldCenter + worldEdges[0] + worldEdges[1] + worldEdges[2]); // this +,+,+
    return results;
}
Rigidbody::Rigidbody() : position(),
                         rotation(0, 0, 0, 1),
                         scale(1.0f),
                         velocity(),
                         angularMomentum(),
                         mass(1.0f),
                         inverseInertiaTensor(computeInertiaTensorInverse(scale, 1 / mass)),
                         worldFromObj(),
                         m_scaledObjToWorld(),
                         m_worldToScaledObj(),
                         frameForce(),
                         frameTorque()
{
}

Rigidbody::Rigidbody(const vec3 center, const vec3 size, float mass) : position(center),
                                                                       rotation(0, 0, 0, 1),
                                                                       scale(size),
                                                                       velocity(),
                                                                       angularMomentum(),
                                                                       mass(mass),
                                                                       inverseInertiaTensor(computeInertiaTensorInverse(scale, 1 / mass)),
                                                                       worldFromObj(),
                                                                       m_scaledObjToWorld(),
                                                                       m_worldToScaledObj(),
                                                                       frameForce(),
                                                                       frameTorque()
{
}

Rigidbody::~Rigidbody()
{
}

void Rigidbody::addForce(const vec3 force, const vec3 where)
{
    frameForce += force;
    frameTorque += glm::cross(where, force);
}

void Rigidbody::addForceWorld(const vec3 force, const vec3 where)
{
    addForce(force, where - position);
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

    mat4 scaleMatrix = glm::scale(mat4(1), scale);
    mat4 translationMatrix = glm::translate(mat4(1), position);
    m_scaledObjToWorld = translationMatrix * rotationMatrix;
    m_worldToScaledObj = glm::inverse(m_scaledObjToWorld);

    worldFromObj = translationMatrix * rotationMatrix * scaleMatrix;
    objFromWorld = glm::inverse(worldFromObj);
}

mat4 Rigidbody::computeInertiaTensorInverse(const vec3 size, float massInverse)
{
    // assumption: homogenous cuboid
    const float x = size.x;
    const float y = size.y;
    const float z = size.z;
    const float xSquared = x * x;
    const float ySquared = y * y;
    const float zSquared = z * z;

    return mat4(
        12.0f * massInverse / (ySquared + zSquared), 0.0f, 0.0f, 0.0f,
        0.0f, 12.0f * massInverse / (xSquared + zSquared), 0.0f, 0.0f,
        0.0f, 0.0f, 12.0f * massInverse / (xSquared + ySquared), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

const vec3 Rigidbody::s_corners[8] =
    {
        vec3(-0.5f, -0.5f, -0.5f),
        vec3(0.5f, -0.5f, -0.5f),
        vec3(-0.5f, 0.5f, -0.5f),
        vec3(0.5f, 0.5f, -0.5f),
        vec3(-0.5f, -0.5f, 0.5f),
        vec3(0.5f, -0.5f, 0.5f),
        vec3(-0.5f, 0.5f, 0.5f),
        vec3(0.5f, 0.5f, 0.5f)};