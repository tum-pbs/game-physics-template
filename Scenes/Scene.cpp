#include "Scene.h"
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Grid.h"
#include <imgui.h>

using namespace glm;

void Scene::onDraw(Renderer &renderer)
{
    Grid grid(2, 2);
    grid(0, 0) = 1;
    grid(0, 1) = 0;
    grid(1, 0) = 0;
    grid(1, 1) = fmod(ImGui::GetTime(), 1.0);
    renderer.drawImage(grid.data, grid.height, grid.width, vec2(0, 0), vec2(0.5, 0.5));
    renderer.drawImage(grid.data, grid.height, grid.width, vec2(0.5, glm::sin(ImGui::GetTime() * 6.0)), vec2(0.1, 0.1));
    renderer.drawCube({0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}, {1, 0, 0}, 0);
}
