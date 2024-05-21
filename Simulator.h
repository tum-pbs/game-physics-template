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
    void init();

private:
    using vec3 = glm::vec3;
    void drawWireCube(vec3 position, vec3 scale, vec3 color);
    void drawCoordinatesAxes();
    void drawPlane(vec3 normal, float distance);
    Renderer &renderer;
};