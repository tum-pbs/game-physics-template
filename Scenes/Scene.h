#pragma once
#include "Renderer.h"
#include <glm/glm.hpp>
#include <vector>

struct Spring
{
    int index1;
    int index2;
    float stiffness;
    float length;
};

class Scene
{
public:
    virtual void init() {};
    virtual void simulateStep() {};
    virtual void onDraw(Renderer &renderer);
    virtual void onGUI() {};
    virtual ~Scene() = default;

protected:
    using vec3 = glm::vec3;
    using quat = glm::quat;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;
    using mat3 = glm::mat3;
    std::vector<vec3> positions;
    std::vector<vec3> velocities;
    std::vector<vec3> tmpPositions;
    std::vector<float> masses;
    std::vector<vec3> forces;
    std::vector<Spring> springs;
    vec3 gravity = {0, 0, 0};
    void resetForces();
    void CalculateForces(std::vector<vec3> &positions_, std::vector<vec3> &forces_, std::vector<Spring> &springs_);
    void integrateMidpoint(float dt);
    void integrateEuler(float dt);
    void printMassPoint(int i);
};
