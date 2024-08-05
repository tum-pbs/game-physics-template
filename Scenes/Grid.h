#pragma once
#include <vector>
#include <iostream>
class Grid
{
public:
    Grid(int height, int width);
    int width;
    int height;
    std::vector<float> data;
    float &operator()(int y, int x);
    friend std::ostream &operator<<(std::ostream &os, Grid &grid);
};