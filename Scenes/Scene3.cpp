#include "Scene3.h"
#include <imgui.h>
#include <stdio.h>
#include "CollisionDetection.h"

void Scene3::init()
{

    rigidbodies.emplace_back(vec3(-1, 0, 0), vec3(1, 1, 1), 100.0f);
    rigidbodies.back().update(0.0f);
    rigidbodies.emplace_back(vec3(1, 1, 0), vec3(1, 2, 1), 100.0f);

    rigidbodies.back().velocity = vec3(-1.0f, 0, 0);
    rigidbodies.back().update(0.0f);
}

void Scene3::simulateStep()
{
    update(0.01f);
}