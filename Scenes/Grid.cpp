#include "Grid.h"
#include <iomanip>

Grid::Grid(size_t width, size_t height, size_t depth) : width(width), height(height), depth(depth)
{
    data.assign(width * height * depth, 0);
}

float &Grid::operator()(size_t x, size_t y, size_t z)
{
    return data[indexOf(x, y, z)];
}

bool Grid::isBorder(size_t x, size_t y, size_t z)
{
    bool left_right = (x == 0 || x == width - 1);
    bool up_down = (y == 0 || y == height - 1);
    bool front_back = ((depth > 1) && (z == 0 || z == depth - 1));
    return left_right || up_down || front_back;
}

size_t Grid::indexOf(size_t x, size_t y, size_t z)
{
    return x + y * width + z * width * height;
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
                os << std::setw(10) << std::left << grid(x, y, z) << " ";
            }
            os << std::endl;
        }
    }
    return os;
}