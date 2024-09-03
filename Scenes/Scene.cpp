#include "Scene.h"
#include <imgui.h>
#include "pcgsolver.h"

using namespace glm;
void Scene::init()
{
    resetGrid();
}

void Scene::onDraw(Renderer &renderer)
{
    if (grid.width > 0)
        renderer.drawImage(grid.data, grid.height, grid.width, 0, 1);
}

void Scene::onGUI()
{
    using namespace ImGui;
    if (Button("Reset"))
    {
        resetGrid();
    }
    SliderInt("Draw Radius", &drawRadius, 1, 10);
    SliderFloat("Alpha", &alpha, 0, 1);
    if (SliderInt("Resolution", (int *)&resolution, 10, 1000))
    {
        resetGrid();
    }
    SliderFloat("dt", &dt, 0, 10, "%.3f", ImGuiSliderFlags_Logarithmic);
    if (IsMouseDown(ImGuiMouseButton_Left) && !GetIO().WantCaptureMouse)
    {
        ImVec2 mousePos = GetMousePos();
        ImVec2 windowSize = GetIO().DisplaySize;
        ImVec2 normalizedMousePos = ImVec2(mousePos.x / (float)windowSize.x, mousePos.y / (float)windowSize.y);
        float x = normalizedMousePos.x * grid.width;
        float y = normalizedMousePos.y * grid.height;
        for (int i = -drawRadius; i < drawRadius; i++)
            for (int j = -drawRadius; j < drawRadius; j++)
            {
                int px = x + 0.5 + i;
                int py = y + 0.5 + j;
                if (px < 0 || px >= grid.width || py < 0 || py >= grid.height)
                    continue;
                if (i * i + j * j < 25)
                    grid(px, py) = 1;
            }
    }
}

/*
 * This function initializes the grid with random values between 0 and 1.
 * The border of the grid is set to 0.
 */
void Scene::randomInit(size_t width, size_t height, size_t depth)
{
    grid = Grid(width, height, depth);
    for (size_t x = 0; x < grid.width; x++)
    {
        for (size_t y = 0; y < grid.height; y++)
        {
            for (size_t z = 0; z < grid.depth; z++)
            {
                if (grid.isBorder(x, y, z))
                    grid(x, y, z) = 0;
                else
                    grid(x, y, z) = rand() / (float)RAND_MAX;
            }
        }
    }
}

void Scene::resetGrid()
{
    randomInit(resolution * Renderer::camera.aspectRatio(), resolution);
}

void Scene::diffuseImplicit(float dt)
{
    size_t N = grid.height * grid.width * grid.depth;
    SparseMatrix<float> A(N);

    float factor = alpha * dt;
    float diffFactor = 4;
    if (grid.depth > 1)
        diffFactor = 6;

    // diagonal to one
    for (size_t i = 0; i < N; i++)
    {
        A.set_element(i, i, 1.0);
    }

    // only the inner grid points
    for (size_t x = 0; x < grid.width; x++)
    {
        for (size_t y = 0; y < grid.height; y++)
        {
            for (size_t z = 0; z < grid.depth; z++)
            {
                if (grid.isBorder(x, y, z))
                    continue;
                size_t index = grid.indexOf(x, y, z);
                A.set_element(index, index, 1 + diffFactor * factor);
                A.add_to_element(index, grid.indexOf(x - 1, y, z), -factor);
                A.add_to_element(index, grid.indexOf(x + 1, y, z), -factor);
                A.add_to_element(index, grid.indexOf(x, y + 1, z), -factor);
                A.add_to_element(index, grid.indexOf(x, y - 1, z), -factor);
                if (grid.depth > 1)
                {
                    A.add_to_element(index, grid.indexOf(x, y, z + 1), -factor);
                    A.add_to_element(index, grid.indexOf(x, y, z - 1), -factor);
                }
            }
        }
    }

    std::vector<float> b(N);
    b.assign(grid.data.begin(), grid.data.end());

    // solve the linear system
    SparsePCGSolver<float> solver;
    solver.set_solver_parameters(1e-5, 1000);

    float residual_out;
    int iterations_out;

    solver.solve(A, b, grid.data, residual_out, iterations_out, 0);
}

void Scene::diffuseExplicit(float dt)
{
    Grid newGrid = Grid(grid.width, grid.height, grid.depth);
    float diffFactor = 4;
    if (grid.depth > 1)
        diffFactor = 6;
    for (size_t x = 0; x < grid.width; x++)
    {
        for (size_t y = 0; y < grid.height; y++)
        {
            for (size_t z = 0; z < grid.depth; z++)
            {
                if (grid.isBorder(x, y, z))
                {
                    newGrid(x, y, z) = grid(x, y, z);
                    continue;
                }
                float total = grid(x + 1, y, z) + grid(x - 1, y, z) + grid(x, y + 1, z) + grid(x, y - 1, z);
                if (grid.depth > 1)
                {
                    total += grid(x, y, z - 1) + grid(x, y, z + 1);
                }
                newGrid(x, y, z) = grid(x, y, z) + dt * alpha * (total - diffFactor * grid(x, y, z));
            }
        }
    }

    grid = newGrid;
}