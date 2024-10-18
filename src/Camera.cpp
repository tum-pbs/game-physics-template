#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

using namespace glm;

Camera::Camera()
    : position(vec3(0)),
      fov(45.0f),
      near(0.01f),
      far(1000.0f),
      width(800),
      height(600)
{
    lookAt(position + worldForward);
}

float Camera::aspectRatio()
{
    return (float)width / (float)height;
}

void Camera::update()
{
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    ImVec2 screenDelta = ImGui::GetMouseDragDelta();
    vec2 angleDelta = vec2(-screenDelta.x, screenDelta.y) * cameraSensitivity;
    vec2 tmpAngles = cameraAngles + angleDelta;
    tmpAngles.y = glm::clamp(tmpAngles.y, -glm::half_pi<float>() + 1e-5f, glm::half_pi<float>() - 1e-5f);
    float cx = cos(tmpAngles.x);
    float sx = sin(tmpAngles.x);
    float cy = cos(tmpAngles.y);
    float sy = sin(tmpAngles.y);
    zoom += ImGui::GetIO().MouseWheel * zoomSensitivity;
    position = vec3(cx * cy, sx * cy, sy) * std::exp(-zoom);
    lookAt(vec3(0.0f));
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        auto delta = ImGui::GetMouseDragDelta();
        cameraAngles = cameraAngles + angleDelta;
    }
}

mat4 Camera::projectionMatrix()
{
    return perspective(radians(fov), aspectRatio(), near, far);
}

vec3 Camera::forward()
{
    return inverse(viewMatrix) * vec4(0, 0, 1, 0);
}

vec3 Camera::up()
{
    return inverse(viewMatrix) * vec4(0, 1, 0, 0);
}

vec3 Camera::right()
{
    return inverse(viewMatrix) * vec4(1, 0, 0, 0);
}

void Camera::lookAt(vec3 target)
{
    viewMatrix = glm::lookAt(position, target, worldUp);
}
