#include "Scene2.h"
#include <imgui.h>
#include <stdio.h>
#include "CollisionDetection.h"

void Scene2::init()
{
    Rigidbody rb = Rigidbody(vec3(0), glm::angleAxis(glm::radians(90.0f), vec3(0, 0, 1)), vec3(1, 0.6, 0.5), 2);
    vec3 force = vec3(1, 1, 0);
    vec3 fwhere = vec3(0.3, 0.5, 0.25);
    rb.addWorldForce(force, fwhere);
    rigidbodies.push_back(rb);
}

void Scene2::simulateStep()
{
    update(0.1f);
}