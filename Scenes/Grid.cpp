#include "Grid.h"

Grid::Grid(int height, int width) : width(width), height(height)
{
    data.assign(width * height, 0);
}

float &Grid::operator()(int y, int x)
{
    return data[y * width + x];
}

std::ostream &operator<<(std::ostream &os, Grid &grid)
{
    for (int y = 0; y < grid.height; y++)
    {
        for (int x = 0; x < grid.width; x++)
        {
            os << grid(y, x) << " ";
        }
        os << std::endl;
    }
    return os;
}