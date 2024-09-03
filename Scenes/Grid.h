#pragma once
#include <vector>
#include <iostream>
class Grid
{
public:
    Grid(size_t width, size_t height, size_t depth = 1);
    size_t width;
    size_t height;
    size_t depth;
    std::vector<float> data;
    float &operator()(size_t x, size_t y, size_t z = 0);
    friend std::ostream &operator<<(std::ostream &os, Grid &grid);
    bool isBorder(size_t x, size_t y, size_t z = 0);
    size_t indexOf(size_t x, size_t y, size_t z = 0);
};