#include "Simulator.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

glm::vec3 rotation(0);
glm::vec3 color(1, 0, 0);
glm::vec3 scale(0.5, 0.5, 0.5);

struct Transform
{
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};
struct Object
{
    Transform transform;
    glm::vec3 color;
};

std::vector<Object> cubes;
void addToCubeCube()
{
    size_t i = cubes.size();
    float n = 5;
    float iz = floor(i / n / n);
    float iy = fmod(floor(i / n), n);
    float ix = fmod(i, n);
    cubes.push_back({{glm::vec3(ix, iy, iz) - (n - 1) / 2, rotation, scale}, color});
}

void Simulator::init()
{
    for (int i = 0; i < 50; i++)
    {
        addToCubeCube();
    }
}

void Simulator::drawCoordinatesAxes()
{
    renderer.drawCube({1, 0, 0}, {0, 0, 0}, {2, 0.1, 0.1}, {1, 0, 0}); // pos x is red
    renderer.drawCube({0, 1, 0}, {0, 0, 0}, {0.1, 2, 0.1}, {0, 1, 0}); // pos y is green
    renderer.drawCube({0, 0, 1}, {0, 0, 0}, {0.1, 0.1, 2}, {0, 0, 1}); // pos z is blue
}

void Simulator::simulateStep()
{
    // float dt = ImGui::GetIO().DeltaTime;
    // rotation.z += 360.0f * dt;
    // rotation.z = fmod(rotation.z, 360.0f);
}

void Simulator::onGUI()
{
    ImGui::Begin("Primitives");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("Color", glm::value_ptr(color));
    ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f);
    ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);
    if (ImGui::Button("Add Cube"))
    {
        addToCubeCube();
    }
    if (ImGui::Button("Remove Cube"))
    {
        cubes.pop_back();
    }
    ImGui::End();
}

void Simulator::onDraw()
{
    drawCoordinatesAxes();
    for (Object cube : cubes)
    {
        renderer.drawCube(cube.transform.position, cube.transform.rotation, cube.transform.scale, cube.color);
    }
    // drawCoordinatesAxes();
    // renderer.drawLine({-1, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1});
    // for (float x = -10; x < 10; x++)
    // {
    //     for (float y = -10; y < 10; y++)
    //     {
    //         for (float z = -10; z < 10; z++)
    //         {
    //             renderer.drawCube({x, y, z}, rotation, scale, color);
    //         }
    //     }
    // }
};