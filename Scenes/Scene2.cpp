#include "Scene2.h"
#include "pcgsolver.h"
#include <iostream>

void Scene2::simulateStep()
{
    diffuseImplicit(dt);
}

void Scene2::diffuseImplicit(float dt)
{
    size_t N = grid.height * grid.width;
    SparseMatrix<float> A(N);

    float factor = alpha * dt;

    // diagonal to one
    for (int i = 0; i < N; i++)
    {
        A.set_element(i, i, 1.0);
    }

    // only the inner grid points
    for (int x = 1; x < grid.width - 1; x++)
    {
        for (int y = 1; y < grid.height - 1; y++)
        {
            int index = y * grid.width + x;
            int leftIndex = index - 1;
            int rightIndex = index + 1;
            int upIndex = index - grid.width;
            int downIndex = index + grid.width;
            A.set_element(index, index, 1 + 4 * factor);
            A.add_to_element(index, leftIndex, -factor);
            A.add_to_element(index, rightIndex, -factor);
            A.add_to_element(index, upIndex, -factor);
            A.add_to_element(index, downIndex, -factor);
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