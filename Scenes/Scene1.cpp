#include "Scene1.h"

void Scene1::simulateStep()
{
    diffuseExplicit(dt);
}

void Scene1::diffuseExplicit(float dt)
{
    Grid newGrid = Grid(grid.width, grid.height);

    for (int x = 1; x < grid.width - 1; x++)
    {
        for (int y = 1; y < grid.height - 1; y++)
        {
            float plusX = grid(x + 1, y);
            float minusX = grid(x - 1, y);
            float plusY = grid(x, y + 1);
            float minusY = grid(x, y - 1);
            newGrid(x, y) = grid(x, y) + dt * alpha * (plusX + minusX + plusY + minusY - 4 * grid(x, y));
        }
    }
    for (int x = 0; x < grid.width; x++)
    {
        newGrid(x, 0) = grid(x, 0);
        newGrid(x, grid.height - 1) = grid(x, grid.height - 1);
    }
    for (int y = 0; y < grid.height; y++)
    {
        newGrid(0, y) = grid(0, y);
        newGrid(grid.width - 1, y) = grid(grid.width - 1, y);
    }
    grid = newGrid;
}