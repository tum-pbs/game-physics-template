#include "Scene.h"
#include <memory>
#include <imgui.h>
#include <stdio.h>

void Scene1::init()
{
    // Initialize the scene
}

void Scene1::simulateStep()
{
    // Simulate the scene
}

void Scene1::onDraw(Renderer &renderer)
{
    // Draw the scene
    renderer.drawCube({0, 0, 0}, glm::quat(1, 0, 0, 0), {1, 1, 1}, {1, 1, 1, 1});
}

void Scene1::onGUI()
{
    // Draw the GUI
    ImGui::Text("hurray");
}

void Scene2::init()
{
}

void Scene2::simulateStep()
{
}

void Scene2::onDraw(Renderer &renderer)
{
    renderer.drawCube({0, 0, 0}, glm::quat(1, 0, 0, 0), {1, 1, 1}, {1, 0, 1, 1});
}

void Scene2::onGUI()
{
}

std::map<std::string, SceneCreator> scenesCreators = {
    {"Scene1", []()
     { return std::make_unique<Scene1>(); }},
    {"Scene2", []()
     { return std::make_unique<Scene2>(); }},
    // add more Scene types here
};