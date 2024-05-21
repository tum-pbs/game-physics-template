#include "Simulator.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/polar_coordinates.hpp>

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
glm::vec3 m_rotation(0, 0, 0);
glm::vec3 m_scale(0.8, 0.8, 0.8);
glm::vec3 m_color(1, 0, 0);
std::vector<Object> cubes;

namespace ImGui
{
    bool DragDirection(const char *label, glm::vec3 &direction)
    {
        glm::vec2 angles = glm::degrees(glm::polar(glm::vec3(direction)));
        bool changed = ImGui::DragFloat2(label, glm::value_ptr(angles));
        direction = glm::euclidean(glm::radians(angles));
        return changed;
    }
}
void addToCubeCube()
{
    size_t i = cubes.size();
    float n = 5;
    float iz = floor(i / n / n);
    float iy = fmod(floor(i / n), n);
    float ix = fmod(i, n);
    cubes.push_back({{glm::vec3(ix, iy, iz) - (n - 1) / 2, m_rotation, m_scale}, m_color});
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
    renderer.drawCube({1, 0, 0}, glm::quat(vec3()), {2, 0.1, 0.1}, {1, 0, 0}); // pos x is red
    renderer.drawCube({0, 1, 0}, glm::quat(vec3()), {0.1, 2, 0.1}, {0, 1, 0}); // pos y is green
    renderer.drawCube({0, 0, 1}, glm::quat(vec3()), {0.1, 0.1, 2}, {0, 0, 1}); // pos z is blue
}

void Simulator::drawWireCube(vec3 position, vec3 scale, vec3 color)
{
    std::vector<vec3> points =
        {
            {0, 0, 0},
            {0, 0, 1},

            {0, 0, 0},
            {0, 1, 0},

            {0, 0, 0},
            {1, 0, 0},

            {0, 0, 1},
            {1, 0, 1},

            {0, 0, 1},
            {0, 1, 1},

            {0, 1, 0},
            {1, 1, 0},

            {0, 1, 0},
            {0, 1, 1},

            {1, 0, 0},
            {1, 1, 0},

            {1, 0, 0},
            {1, 0, 1},

            {1, 1, 0},
            {1, 1, 1},

            {1, 0, 1},
            {1, 1, 1},

            {0, 1, 1},
            {1, 1, 1}};
    for (vec3 &point : points)
    {
        point -= 0.5;
        point *= scale;
        point += position;
    }
    for (int i = 0; i < points.size() / 2; i++)
    {
        renderer.drawLine(points[i * 2], points[i * 2 + 1], color);
    }
}

void Simulator::simulateStep()
{
    // float dt = ImGui::GetIO().DeltaTime;
    // rotation.z += 360.0f * dt;
    // rotation.z = fmod(rotation.z, 360.0f);
}

void Simulator::drawPlane(vec3 normal, float distance)
{
    vec3 center = normal * distance;
    renderer.drawLine(center, center + normal, {1, 1, 1});
    float size = 5;
    vec3 forward = glm::cross(normal, vec3(1, 0, 0));
    vec3 right = glm::cross(normal, forward);
    forward = glm::normalize(forward);
    right = glm::normalize(right);
    renderer.drawLine(center - forward * size + right * size, center + forward * size + right * size, {0, 0, 0});
    renderer.drawLine(center + forward * size + right * size, center + forward * size - right * size, {0, 0, 0});
    renderer.drawLine(center + forward * size - right * size, center - forward * size - right * size, {0, 0, 0});
    renderer.drawLine(center - forward * size - right * size, center - forward * size + right * size, {0, 0, 0});
    glm::quat rotation = glm::quat_cast(glm::mat3(forward, right, normal));
    renderer.drawQuad(center - 0.001f * normal, rotation, vec3(size * 2), {0.0f, 0.0f, 0.0f, 0.5f}, Renderer::unlit);
}

void Simulator::onGUI()
{
    ImGui::Begin("Primitives");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("Color", glm::value_ptr(m_color));
    ImGui::DragFloat3("Rotation", glm::value_ptr(m_rotation), 0.1f);
    ImGui::DragFloat3("Scale", glm::value_ptr(m_scale), 0.01f);

    ImGui::BeginTable("Cubes", 2);
    ImGui::PushButtonRepeat(true);
    ImGui::TableNextColumn();
    if (ImGui::Button("Add Cube"))
    {
        addToCubeCube();
    }
    ImGui::TableNextColumn();
    if (ImGui::Button("Remove Cube"))
    {
        if (!cubes.empty())
            cubes.pop_back();
    }
    ImGui::EndTable();
    ImGui::CheckboxFlags("Culling Plane Enabled", &renderer.m_uniforms.flags, Renderer::cullingPlane);
    if (renderer.m_uniforms.flags)
    {
        ImGui::DragDirection("CullDirection", renderer.m_uniforms.cullingNormal);
        ImGui::DragFloat("CullOffset", &renderer.m_uniforms.cullingOffset, 0.01f);
    }
    ImGui::End();
}

void Simulator::onDraw()
{
    drawCoordinatesAxes();
    drawWireCube({0, 0, 0}, {5, 5, 5}, {1, 1, 1});
    if (renderer.m_uniforms.flags & 1)
        drawPlane(renderer.m_uniforms.cullingNormal, renderer.m_uniforms.cullingOffset);
    for (Object &cube : cubes)
    {
        renderer.drawCube(cube.transform.position, glm::quat(glm::radians(cube.transform.rotation)), cube.transform.scale, cube.color);
    }
};