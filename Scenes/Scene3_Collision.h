#pragma once
#include "Scene.h"
#include "RigidBody.h"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

class Scene3_Collision : public Scene {
private:
    RigidBody* body1;
    RigidBody* body2;
    float timeStep = 0.02f; 
    float restitution = 0.8f; 

public:
    void init() override {
        glm::vec3 position1 = glm::vec3(-2.0f, 0.0f, 0.0f);
        glm::vec3 position2 = glm::vec3(2.0f, 0.0f, 0.0f);
        glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
        float mass = 2.0f;

        float width = 1.0f, height = 1.0f, depth = 1.0f;

        body1 = new RigidBody(position1, orientation, mass, width, height, depth);
        body2 = new RigidBody(position2, orientation, mass, width, height, depth);

        body1->velocity = glm::vec3(1.0f, 0.0f, 0.0f); 
        body2->velocity = glm::vec3(-1.0f, 0.0f, 0.0f); 
    }

    void simulateStep(float deltaTime) {
        body1->integrate(deltaTime);
        body2->integrate(deltaTime);

        if (checkCollision()) {
            handleCollision();
        }
    }

    bool checkCollision() {
        glm::vec3 diff = body2->position - body1->position;
        float distance = glm::length(diff);
        float collisionDistance = 1.0f;
        return distance <= collisionDistance;
    }

    void handleCollision() {

        glm::vec3 normal = glm::normalize(body2->position - body1->position);

        glm::vec3 relativeVelocity = body2->velocity - body1->velocity;

        float impulseNumerator = -(1.0f + restitution) * glm::dot(relativeVelocity, normal);
        float impulseDenominator = (1.0f / body1->mass) + (1.0f / body2->mass);
        float impulse = impulseNumerator / impulseDenominator;

        body1->velocity += (impulse * normal) / body1->mass;
        body2->velocity -= (impulse * normal) / body2->mass;
    }

    void onDraw(Renderer& renderer) override {
        simulateStep(timeStep); 
        renderer.drawWireCube(body1->position, glm::vec3(1.0f), glm::vec3(1.0f));
        renderer.drawWireCube(body2->position, glm::vec3(1.0f), glm::vec3(1.0f));
    }

    ~Scene3_Collision() {
        delete body1;
        delete body2;
    }
};
