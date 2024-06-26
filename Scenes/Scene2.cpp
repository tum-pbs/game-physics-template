#include "Scene2.h"
#include <imgui.h>

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