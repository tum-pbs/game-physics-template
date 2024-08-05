#include "Scene.h"
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Grid.h"
#include <imgui.h>

using namespace glm;

void Scene::onDraw(Renderer &renderer)
{
    if (grid.width > 0)
        renderer.drawImage(grid.data, grid.height, grid.width, vec2(0, 0), vec2(1.0, 1.0));
}

void Scene::randomInit(size_t width, size_t height)
{
    grid = Grid(height, width);
    for (size_t i = 0; i < width * height; i++)
    {
        grid.data[i] = rand() / (float)RAND_MAX;
    }
}
