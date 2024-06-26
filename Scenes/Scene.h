#pragma once
#include "Renderer.h"

class Scene
{
public:
    virtual void init() {};
    virtual void simulateStep() {};
    virtual void onDraw(Renderer &renderer) { (void)renderer; };
    virtual void onGUI() {};
    virtual ~Scene() = default;
};
