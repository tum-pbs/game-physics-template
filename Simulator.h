#pragma once
#include "Renderer.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

class Simulator
{
public:
    Simulator(Renderer &renderer) : renderer(renderer)
    {
        renderer.defineGUI = [this]()
        { onGUI(); };
    };
    void simulateStep();
    void onDraw();
    void onGUI();

private:
    void drawCoordinatesAxes();
    Renderer &renderer;
};