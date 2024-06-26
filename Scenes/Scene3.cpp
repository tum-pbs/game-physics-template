#include "Scene3.h"
#include <imgui.h>

void Scene3::init()
{
    vec3 position1 = {0, 0, 0};
    vec3 position2 = {0, 2, 0};
    vec3 velocity1 = {-1, 0, 0};
    vec3 velocity2 = {1, 0, 0};
    float mass1 = 10;
    float mass2 = 10;
    float L = 1;
    float k = 40;
    positions.clear();
    velocities.clear();
    tmpPositions.clear();
    masses.clear();
    forces.clear();
    springs.clear();
    positions.push_back(position1);
    positions.push_back(position2);
    velocities.push_back(velocity1);
    velocities.push_back(velocity2);
    masses.push_back(mass1);
    masses.push_back(mass2);
    Spring spring = {0, 1, k, L};
    springs.push_back(spring);
    tmpPositions.resize(positions.size());
    forces.resize(positions.size());
}

void Scene3::simulateStep()
{
    if (!pause)
        integrateMidpoint(0.005f);
}

void Scene3::onGUI()
{
    ImGui::Checkbox("Pause", &pause);
}