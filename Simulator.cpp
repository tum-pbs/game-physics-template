#include "Simulator.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

glm::vec3 rotation(0);
glm::vec3 color(1, 0, 0);
glm::vec3 scale(0.5, 0.5, 0.5);

void Simulator::drawCoordinatesAxes()
{
    renderer.drawCube({1, 0, 0}, {0, 0, 0}, {2, 0.1, 0.1}, {1, 0, 0}); // pos x is red
    renderer.drawCube({0, 1, 0}, {0, 0, 0}, {0.1, 2, 0.1}, {0, 1, 0}); // pos y is green
    renderer.drawCube({0, 0, 1}, {0, 0, 0}, {0.1, 0.1, 2}, {0, 0, 1}); // pos z is blue
}

void Simulator::simulateStep()
{
    // Nothing to do here
    rotation.z += 0.1f;
    rotation.z = fmod(rotation.z, 360.0f);
}

void Simulator::onGUI()
{
    ImGui::Begin("Primitives");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("Color", glm::value_ptr(color));
    ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.1f);
    ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f);
    ImGui::End();
}

void Simulator::onDraw()
{
    drawCoordinatesAxes();
    renderer.drawLine({-1, 0, 1}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1});
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