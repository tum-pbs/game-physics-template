#pragma once
#include "Renderer.h"
#include <glm/glm.hpp>
#include <vector>
#include "Rigidbody.h"

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
    vec3 gravity = {0, 0, 0};
    std::vector<Rigidbody> rigidbodies;
    void update(float deltaTime);
};
