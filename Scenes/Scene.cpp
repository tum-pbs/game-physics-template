#include "Scene.h"
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>

void Scene::onDraw(Renderer &renderer)
{
    for (int i = 0; i < positions.size(); i++)
    {
        renderer.drawSphere(positions[i], 0.1f, {1, 0, 0});
    }
    for (auto &spring : springs)
    {
        renderer.drawLine(positions[spring.index1], positions[spring.index2], {0, 1, 0});
    }
}

void Scene::printMassPoint(int i)
{
    using namespace std;
    cout << "Mass Point " << i << ":" << endl;
    cout << "position: " << glm::to_string(positions[i]) << endl;
    cout << "velocity: " << glm::to_string(velocities[i]) << endl;
    cout << "mass: " << masses[i] << endl;
}

void Scene::integrateMidpoint(float dt)
{
    resetForces();
    CalculateForces(positions, forces, springs);
    for (int i = 0; i < positions.size(); i++)
    {
        vec3 a = forces[i] / masses[i];
        vec3 tmpV = velocities[i] + a * dt / 2.0f;
        tmpPositions[i] = positions[i] + velocities[i] * dt / 2.0f;
        positions[i] += tmpV * dt;
    }
    resetForces();
    CalculateForces(tmpPositions, forces, springs);
    for (int i = 0; i < positions.size(); i++)
    {
        vec3 a = forces[i] / masses[i];
        velocities[i] += a * dt;
    }
}

void Scene::integrateEuler(float dt)
{
    resetForces();
    CalculateForces(positions, forces, springs);
    for (int i = 0; i < positions.size(); i++)
    {
        vec3 a = forces[i] / masses[i];
        positions[i] += velocities[i] * dt;
        velocities[i] += a * dt;
    }
}

void Scene::resetForces()
{
    for (auto &force : forces)
    {
        force = {0, 0, 0};
    }
}

void Scene::CalculateForces(std::vector<vec3> &positions_, std::vector<vec3> &forces_, std::vector<Spring> &springs_)
{
    for (auto &spring : springs_)
    {
        vec3 p1 = positions_[spring.index1];
        vec3 p2 = positions_[spring.index2];
        vec3 d = p2 - p1;
        float l = glm::length(d);
        vec3 f = spring.stiffness * (l - spring.length) * glm::normalize(d);
        forces_[spring.index1] += f;
        forces_[spring.index2] -= f;
    }
    for (auto &force : forces_)
    {
        force += gravity;
    }
}