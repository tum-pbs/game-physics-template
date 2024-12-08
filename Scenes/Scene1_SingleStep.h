#pragma once
#include "Scene.h"
#include "RigidBody.h"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

class Scene1_SingleStep : public Scene {
private:
    RigidBody* body;

public:
    void init() override {
        glm::vec3 initialPosition = glm::vec3(0.0f);
        glm::quat initialOrientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        float mass = 2.0f;

        // Define the dimensions of the cuboid
        float width = 1.0f;  // Replace with actual width
        float height = 0.6f; // Replace with actual height
        float depth = 0.5f;  

        // Create the RigidBody with the new constructor
        body = new RigidBody(initialPosition, initialOrientation, mass, width, height, depth);

        // Apply force and calculate torque in the world frame
        glm::vec3 force = glm::vec3(1.0f, 1.0f, 0.0f);
        glm::vec3 applicationPoint = glm::vec3(0.3f, 0.5f, 0.25f);
        body->applyForce(force, applicationPoint);

        simulateStep(2.0f);
    }

    void simulateStep(float deltaTime) {
        body->integrate(deltaTime);

        glm::vec3 testPoint = glm::vec3(-0.3f, -0.5f, -0.25f);
        glm::vec3 pointVel = body->pointVelocity(testPoint);

        std::cout << "Position: " << glm::to_string(body->position) << std::endl;
        std::cout << "Velocity: " << glm::to_string(body->velocity) << std::endl;
        std::cout << "Orientation: " << glm::to_string(body->orientation) << std::endl;
        std::cout << "Angular Velocity: " << glm::to_string(body->angularVelocity) << std::endl;
        std::cout << "Point Velocity: " << glm::to_string(pointVel) << std::endl;
    }

    void onDraw(Renderer& renderer) override {
        renderer.drawWireCube(body->position, glm::vec3(1.0f), glm::vec3(1.0f));
    }

    ~Scene1_SingleStep() {
        delete body;
    }
};

