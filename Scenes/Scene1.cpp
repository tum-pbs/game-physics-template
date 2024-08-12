#include "Scene1.h"

void Scene1::simulateStep()
{
    diffuseExplicit(dt);
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