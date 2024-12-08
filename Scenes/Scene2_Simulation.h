#pragma once
#include "Scene.h"
#include "RigidBody.h"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

class Scene2_Simulation : public Scene {
private:
    RigidBody* body; 
    float timeStep = 0.01f;   
    glm::vec3 appliedForce;   

public:
    void init() override {
        glm::vec3 initialPosition = glm::vec3(0.0f, 5.0f, 0.0f); 
        glm::quat initialOrientation = glm::quat(); 
        float mass = 2.0f;

        float width = 1.0f, height = 0.6f, depth = 0.5f;

        body = new RigidBody(initialPosition, initialOrientation, mass, width, height, depth);
        appliedForce = glm::vec3(0.0f); 
    }

    void applyForces() {
        glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
        body->applyForce(gravity * body->mass, body->position);

        if (glm::length(appliedForce) > 0.0f) {
            body->applyForce(appliedForce, body->position);
            appliedForce = glm::vec3(0.0f); 
        }
    }

    void handleInput() {
        if (ImGui::Button("Push Up")) {
            appliedForce = glm::vec3(0.0f, 20.0f, 0.0f);
        }
        if (ImGui::Button("Push Left")) {
            appliedForce = glm::vec3(-10.0f, 0.0f, 0.0f);
        }
        if (ImGui::Button("Push Right")) {
            appliedForce = glm::vec3(10.0f, 0.0f, 0.0f);
        }
        if (ImGui::Button("Push Forward")) {
            appliedForce = glm::vec3(0.0f, 0.0f, -10.0f);
        }
        if (ImGui::Button("Push Backward")) {
            appliedForce = glm::vec3(0.0f, 0.0f, 10.0f);
        }
    }

    void simulateStep(float deltaTime) {
        applyForces();        
        body->integrate(deltaTime); 

        if (body->position.y <= 0.0f) {
            body->position.y = 0.0f; 
            body->velocity.y *= -0.8f; 
        }
    }

    void onDraw(Renderer& renderer) override {
        simulateStep(timeStep); 
        renderer.drawWireCube(body->position, glm::vec3(1.0f), glm::vec3(1.0f));
    }

    void onGUI() override {
        ImGui::Text("Single Rigid Body Simulation");
        ImGui::SliderFloat("Time Step", &timeStep, 0.001f, 0.1f); 
        handleInput();
    }

    ~Scene2_Simulation() {
        delete body;
    }
};
