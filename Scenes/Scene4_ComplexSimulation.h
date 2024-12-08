#pragma once
#include "Scene.h"
#include "RigidBody.h"
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <vector>

class Scene4_ComplexSimulation : public Scene {
private:
    std::vector<RigidBody*> bodies; 
    glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f); 
    float timeStep = 0.02f; 
    float restitution = 0.8f; 

public:
    void init() override {
        float mass = 2.0f;
        float width = 1.0f, height = 1.0f, depth = 1.0f;

        bodies.push_back(new RigidBody(glm::vec3(-2.0f, 5.0f, 0.0f), glm::quat(), mass, width, height, depth));
        bodies.push_back(new RigidBody(glm::vec3(2.0f, 8.0f, 0.0f), glm::quat(), mass, width, height, depth));
        bodies.push_back(new RigidBody(glm::vec3(0.0f, 10.0f, -2.0f), glm::quat(), mass, width, height, depth));
        bodies.push_back(new RigidBody(glm::vec3(-1.0f, 12.0f, 2.0f), glm::quat(), mass, width, height, depth));
        bodies[0]->velocity = glm::vec3(1.0f, -1.0f, 0.0f);
        bodies[1]->velocity = glm::vec3(-1.0f, -2.0f, 0.0f);
        bodies[2]->velocity = glm::vec3(0.0f, -1.5f, 1.0f);
        bodies[3]->velocity = glm::vec3(0.5f, -2.0f, -0.5f);
    }

    void applyForces() {
        glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f); 
        for (auto& body : bodies) {
            body->applyForce(gravity * body->mass, body->position); 
        }
    }

    void handleCollisions() {
        for (auto& body : bodies) {

            if (body->position.y <= 0.0f) {
                body->position.y = 0.0f; 
                body->velocity.y *= -restitution; 
            }

            if (body->position.x <= -10.0f || body->position.x >= 10.0f) {
                body->velocity.x *= -restitution; 
            }
            if (body->position.z <= -10.0f || body->position.z >= 10.0f) {
                body->velocity.z *= -restitution; 
            }
        }
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                if (checkCollision(bodies[i], bodies[j])) {
                    handleBodyCollision(bodies[i], bodies[j]);
                }
            }
        }
    }

    bool checkCollision(RigidBody* body1, RigidBody* body2) {
        glm::vec3 diff = body2->position - body1->position;
        float distance = glm::length(diff);
        float collisionDistance = 2.0f;
        return distance <= collisionDistance;
    }

    void handleBodyCollision(RigidBody* body1, RigidBody* body2) {
        glm::vec3 normal = glm::normalize(body2->position - body1->position);
        glm::vec3 relativeVelocity = body2->velocity - body1->velocity;
        float impulseNumerator = -(1.0f + restitution) * glm::dot(relativeVelocity, normal);
        float impulseDenominator = (1.0f / body1->mass) + (1.0f / body2->mass);
        float impulse = impulseNumerator / impulseDenominator;
        body1->velocity += (impulse * normal) / body1->mass;
        body2->velocity -= (impulse * normal) / body2->mass;
    }

    void simulateStep(float deltaTime) {
        applyForces();
        for (auto& body : bodies) {
            body->integrate(deltaTime); 
        }
        handleCollisions();
    }

    void onDraw(Renderer& renderer) override {
        simulateStep(timeStep); 
        for (auto& body : bodies) {
            renderer.drawWireCube(body->position, glm::vec3(1.0f), glm::vec3(1.0f));
        }
    }

    ~Scene4_ComplexSimulation() {
        for (auto& body : bodies) {
            delete body;
        }
    }
};
