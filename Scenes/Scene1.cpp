#include "Scene1.h"
#include <imgui.h>
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include "Renderer.h"

void Scene1::init()
{
    resetGrid();
}

void Scene1::simulateStep()
{
    diffuseExplicit(dt);
}

void Scene1::onGUI()
{
    ImGui::SliderFloat("Alpha", &alpha, 0, 1);
    if (ImGui::Button("Reset"))
    {
        resetGrid();
    }
    if (ImGui::SliderInt("Resolution", (int *)&resolution, 10, 1000))
    {
        resetGrid();
    }
    ImGui::SliderFloat("dt", &dt, 0, 10, "%.3f", ImGuiSliderFlags_Logarithmic);
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 normalizedMousePos = ImVec2((mousePos.x) / Renderer::camera.width, (mousePos.y) / Renderer::camera.height);
        int x = normalizedMousePos.x * resolution * Renderer::camera.aspectRatio();
        int y = normalizedMousePos.y * resolution;
        for (int i = -5; i < 5; i++)
            for (int j = -5; j < 5; j++)
            {
                int px = x + i;
                int py = y + j;
                if (i * i + j * j < 25)
                    grid(py, px) = 1;
            }
    }
}

void Scene1::diffuseExplicit(float dt)
{
    Grid newGrid = Grid(grid.height, grid.width);
    for (int x = 1; x < grid.width - 1; x++)
    {
        for (int y = 1; y < grid.height - 1; y++)
        {
            float plusX = grid(y, x + 1);
            float minusX = grid(y, x - 1);
            float plusY = grid(y + 1, x);
            float minusY = grid(y - 1, x);
            newGrid(y, x) = grid(y, x) + dt * alpha * (plusX + minusX + plusY + minusY - 4 * grid(y, x));
        }
    }
    grid = newGrid;
}

void Scene1::resetGrid()
{
    grid = Grid(resolution, resolution * Renderer::camera.aspectRatio());
    randomInit(resolution * Renderer::camera.aspectRatio(), resolution);
}
