#pragma once
#include "Renderer.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Scenes/Scene.h"
#include <memory>

class Simulator
{
public:
    Simulator(Renderer &renderer) : renderer(renderer)
    {
        renderer.defineGUI = [this]()
        { onGUI(); };
    };
    void simulateStep();
    void onDraw();
    void onGUI();
    void init();

private:
    using vec3 = glm::vec3;
    using vec2 = glm::vec2;
    Renderer &renderer;
    std::unique_ptr<Scene> currentScene;
    std::string currentSceneName;
    std::vector<std::string> sceneNames;
    double lastStepTime = 0;
    double lastDrawPrepTime = 0;
    bool limitFPS = true;

    void updateCamera();
    vec2 cameraAngles = vec2(-0.1, 0.2);
    float zoom = -2;
    float zoomSensitivity = 0.1;
    float cameraSensitivity = 0.01;
};