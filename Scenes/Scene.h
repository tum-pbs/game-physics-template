#pragma once
#include "Renderer.h"

class Scene
{
public:
    virtual void init() = 0;
    virtual void simulateStep() = 0;
    virtual void onDraw(Renderer &renderer) = 0;
    virtual void onGUI() = 0;
    virtual ~Scene() = default;
};
