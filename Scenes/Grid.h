#include <vector>
class Grid
{
public:
    Grid(int height, int width) : width(width), height(height)
    {
        data.resize(width * height);
    }
    int width;
    int height;
    std::vector<float> data;
    float &operator()(int y, int x)
    {
        return data[y * width + x];
    }
};