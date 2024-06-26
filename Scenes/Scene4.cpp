#include "Scene4.h"
#include <imgui.h>

void Scene4::initCube(int res, float stiffness)
{
    positions.clear();
    velocities.clear();
    tmpPositions.clear();
    masses.clear();
    forces.clear();
    springs.clear();

    float L = 1.0f / (res - 1);
    for (int x = 0; x < res; x++)
    {
        for (int y = 0; y < res; y++)
        {
            for (int z = 0; z < res; z++)
            {
                vec3 pos = {x, y, z};
                pos = pos / (res - 1.0f) - 0.5f;
                positions.push_back(pos);
                velocities.push_back(glm::vec3(0, 0, 0));
                masses.push_back(1);
            }
        }
    }
    for (int i = 0; i < positions.size(); i++)
    {
        for (int j = i + 1; j < positions.size(); j++)
        {
            float dist = glm::distance(positions[i], positions[j]);
            if (dist < 1.1f * L)
            {
                Spring spring = {i, j, stiffness, L};
                springs.push_back(spring);
            }
        }
    }
    tmpPositions.resize(positions.size());
    forces.resize(positions.size());
}

void Scene4::init()
{
    initCube(3, 40);
    gravity = {0, 0, -1};
}

void Scene4::simulateStep()
{
    float realtimeDt = ImGui::GetIO().DeltaTime;
    dt = realtimeDt * timescale;
    if (!pause)
    {
        if (integrator == "Euler")
        {
            integrateEuler(dt);
        }
        else if (integrator == "Midpoint")
        {
            integrateMidpoint(dt);
        }
    }
    resolveCollisions();
}

void Scene4::onGUI()
{
    if (ImGui::BeginCombo("Integrator", integrator.c_str()))
    {
        if (ImGui::Selectable("Euler", integrator == "Euler"))
        {
            integrator = "Euler";
        }
        if (ImGui::Selectable("Midpoint", integrator == "Midpoint"))
        {
            integrator = "Midpoint";
        }
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Pause", &pause);
    if (ImGui::Button("Reset"))
    {
        initCube(cubeRes, 40);
    }
    ImGui::DragFloat("timescale", &timescale, 0.1f, 0.0001f, 100.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
    ImGui::Text("Physics dt: %.4f", dt);
    ImGui::DragFloat3("Gravity", glm::value_ptr(gravity), 0.01f, -50.0f, 50.0f, "%.2f");
    if (ImGui::DragInt("Cube Resolution", &cubeRes, 1, 1, 20))
    {
        initCube(cubeRes, 40);
    }
}

void Scene4::resolveCollisions()
{
    vec3 boxSize = {5, 5, 5};
    vec3 position = {0, 0, 0};
    vec3 mins = position - boxSize / 2.0f;
    vec3 maxs = position + boxSize / 2.0f;
    for (int i = 0; i < positions.size(); i++)
    {
        vec3 &pos = positions[i];
        vec3 &vel = velocities[i];

        for (int j = 0; j < 3; j++)
        {
            if (pos[j] < mins[j])
            {
                pos[j] = mins[j];
                vel[j] = -vel[j];
            }
            if (pos[j] > maxs[j])
            {
                pos[j] = maxs[j];
                vel[j] = -vel[j];
            }
        }
    }
}