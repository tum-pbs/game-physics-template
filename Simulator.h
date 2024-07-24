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
    Renderer &renderer;
    std::unique_ptr<Scene> currentScene;
    std::string currentSceneName;
    std::vector<std::string> sceneNames;
    double lastStepTime = 0;
    double lastDrawPrepTime = 0;
    bool limitFPS = true;
};