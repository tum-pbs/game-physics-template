#include "Scene3.h"
#include <imgui.h>

void Scene3::init()
{
    resolution = 10;
    resetGrid();
}

void Scene3::simulateStep()
{
    switch (solver)
    {
    case (Solver::Explicit):
        diffuseExplicit(dt);
        break;
    case (Solver::Implicit):
        diffuseImplicit(dt);
        break;
    }
}

void Scene3::onDraw(Renderer &renderer)
{
    glm::vec3 gridDimensions(grid.width, grid.height, grid.depth);
    Colormap cmap("hot");
    int excludeSize = (!drawBorder) * 2;

    renderer.drawWireCube(glm::vec3(0), glm::vec3(scale));
    for (size_t x = 0; x < grid.width - excludeSize; x++)
    {
        for (size_t y = 0; y < grid.height - excludeSize; y++)
        {
            for (size_t z = 0; z < grid.depth - excludeSize; z++)
            {
                glm::vec3 position = (glm::vec3(x, y, z) + 0.5f) / (gridDimensions - (float)excludeSize);
                position -= 0.5f;
                position *= scale;
                size_t d = excludeSize / 2;
                float temperature = grid(x + d, y + d, z + d);
                glm::vec3 c = cmap(temperature);
                float alpha = doCulling ? 1.0f : temperature;
                glm::vec4 color = glm::vec4(c.x, c.y, c.z, alpha);
                renderer.drawCube(position, glm::quat(glm::vec3(0)), scale / (gridDimensions - (float)excludeSize), color, Renderer::DrawFlags::unlit);
            }
        }
    }
    if (!doCulling)
        renderer.enableDepthSorting();
    else
        renderer.drawCullingPlanes(cullingOffsets);
}

void Scene3::onGUI()
{
    using namespace ImGui;
    if (BeginCombo("Solver", solver == Solver::Explicit ? "Explicit" : "Implicit"))
    {

        if (ImGui::Selectable("Explicit", solver == Solver::Explicit))
        {
            solver = Solver::Explicit;
        }
        if (ImGui::Selectable("Implicit", solver == Solver::Implicit))
        {
            solver = Solver::Implicit;
        }
        EndCombo();
    }
    if (Button("Reset"))
    {
        resetGrid();
    }
    Checkbox("Draw Border", &drawBorder);
    Checkbox("Enable Culling Planes", &doCulling);
    if (doCulling)
    {
        DragFloat3("Offsets", glm::value_ptr(cullingOffsets), scale / (float)(resolution - 2 * (!drawBorder)), -scale / 2.0f, scale / 2.0f);
    }
    SliderFloat("Alpha", &alpha, 0, 1);
    if (SliderInt("Resolution", (int *)&resolution, 10, 25))
    {
        resetGrid();
    }
    SliderFloat("dt", &dt, 0, 10, "%.3f", ImGuiSliderFlags_Logarithmic);
}

void Scene3::resetGrid()
{
    randomInit(resolution, resolution, resolution);
}
