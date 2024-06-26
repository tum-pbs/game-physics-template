#include "Scene1.h"
#include <imgui.h>

void Scene1::onDraw(Renderer &renderer)
{
    renderer.drawCube({0, 0, 0}, glm::quat(1, 0, 0, 0), {1, 1, 1}, {1, 1, 1, 1});
}

void Scene1::onGUI()
{
}