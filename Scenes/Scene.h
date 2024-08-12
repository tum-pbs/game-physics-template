#pragma once
#include <vector>
#include "Renderer.h"
#include "Grid.h"

class Scene
{
public:
    virtual void init();
    virtual void simulateStep() {};
    virtual void onDraw(Renderer &renderer);
    virtual void onGUI();
    virtual ~Scene() = default;
    Grid grid = Grid(0, 0);

protected:
    void randomInit(size_t width, size_t height);
    float alpha = 1;
    float dt = 0.01;
    int drawRadius = 5;
    size_t resolution = 50;
    void resetGrid();
};
