#include "Grid.h"

Grid::Grid(size_t width, size_t height, size_t depth) : width(width), height(height), depth(depth)
{
    data.assign(width * height * depth, 0);
}

float &Grid::operator()(size_t x, size_t y, size_t z)
{
    return data[x + y * width + z * width * height];
}

std::ostream &operator<<(std::ostream &os, Grid &grid)
{
    for (size_t z = 0; z < grid.depth; z++)
    {
        os << "z=" << z << ":" << std::endl;
        for (size_t y = 0; y < grid.height; y++)
        {
            for (size_t x = 0; x < grid.width; x++)
            {
                os << grid(x, y, z) << " ";
            }
            os << std::endl;
        }
    }
    return os;
}