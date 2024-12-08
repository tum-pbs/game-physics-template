#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp> 

class RigidBody {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat orientation;
    glm::vec3 angularVelocity;
    float mass;
    glm::mat3 inertiaTensor;
    glm::mat3 inertiaTensorInv;

    RigidBody(glm::vec3 initialPosition, glm::quat initialOrientation, float m, float width, float height, float depth)
        : position(initialPosition),
          velocity(glm::vec3(0.0f)),
          orientation(initialOrientation),
          angularVelocity(glm::vec3(0.0f)),
          mass(m) {

        float Ixx = (1.0f / 12.0f) * mass * (height * height + depth * depth);
        float Iyy = (1.0f / 12.0f) * mass * (width * width + depth * depth);
        float Izz = (1.0f / 12.0f) * mass * (width * width + height * height);

        inertiaTensor = glm::mat3(
            Ixx, 0.0f, 0.0f,
            0.0f, Iyy, 0.0f,
            0.0f, 0.0f, Izz
        );

        inertiaTensorInv = glm::inverse(inertiaTensor);
    }
    void applyForce(const glm::vec3& force, const glm::vec3& point) {
        glm::vec3 r = point - position;
        glm::vec3 torque = glm::cross(r, force);

        velocity += (force / mass);
        //angularVelocity += inertiaTensorInv * torque;
    }

    void integrate(float deltaTime) {
        position += velocity * deltaTime;
    
        glm::quat angularQuat(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
        orientation += 0.5f * angularQuat * orientation * deltaTime;
        orientation = glm::normalize(orientation);
        glm::mat3 rotationMatrix = glm::toMat3(orientation);
        glm::mat3 worldInertiaTensorInv = rotationMatrix * inertiaTensorInv * glm::transpose(rotationMatrix);
        angularVelocity = worldInertiaTensorInv * angularVelocity;
    }

    glm::vec3 pointVelocity(const glm::vec3& point) const {
        glm::vec3 r = point - position;
        return velocity + glm::cross(angularVelocity, r);
    }
};
