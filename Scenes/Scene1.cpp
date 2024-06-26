#include "Scene1.h"
#include <imgui.h>
#include <stdio.h>

void Scene1::resetValues()
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

void Scene1::init()
{
    resetValues();
    std::cout << "Initial Values:" << std::endl;
    printMassPoint(0);
    printMassPoint(1);
    integrateMidpoint(0.1f);
    std::cout << "After 0.1s using Midpoint Method:" << std::endl;
    printMassPoint(0);
    printMassPoint(1);
    resetValues();
    integrateEuler(0.1f);
    std::cout << "After 0.1s using Euler Method:" << std::endl;
    printMassPoint(0);
    printMassPoint(1);
}
