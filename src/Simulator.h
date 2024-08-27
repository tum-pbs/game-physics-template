#pragma once
#include "Renderer.h"
#include "glm/glm.hpp"
#include "Scenes/Scene.h"

/// @brief Backend for running and selecting different scenes.
class Simulator
{
public:
    Simulator(Renderer &renderer) : renderer(renderer)
    {
        renderer.defineGUI = [this]()
        { onGUI(); };
    };

    /// @brief Call simulateStep for the currently active Scene
    void simulateStep();
    /// @brief Call onDraw for the currently active Scene
    void onDraw();
    /// @brief Call onGUI for the currently active Scene, add scene selection and rendering options
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
};