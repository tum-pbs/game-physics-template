#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

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
