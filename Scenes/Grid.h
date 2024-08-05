#pragma once
#include <vector>
class Grid
{
public:
    Grid(int height, int width) : width(width), height(height)
    {
        data.assign(width * height, 0);
    }
    int width;
    int height;
    std::vector<float> data;
    float &operator()(int y, int x)
    {
        return data[y * width + x];
    }
};