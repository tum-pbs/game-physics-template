#include "Scene2.h"
#include <imgui.h>
#include <stdio.h>
#include "CollisionDetection.h"

void Scene2::init()
{
    rigidbodies.emplace_back(vec3(0), vec3(1, 0.6, 0.5), 2);
    rigidbodies.back().rotation = glm::angleAxis(glm::radians(90.0f), vec3(0, 0, 1));
    rigidbodies.back().update(0.0f);

    vec3 force = vec3(1.0);
    vec3 fwhere = vec3(0.3, 0.5, 0.25);
    rigidbodies.back().addForceWorld(force, fwhere);
}

void Scene2::simulateStep()
{
    update(0.1f);
}