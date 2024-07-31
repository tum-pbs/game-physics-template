#include "Scene3.h"
#include <imgui.h>
#include <stdio.h>
#include "CollisionDetection.h"

void Scene3::init()
{
    Rigidbody rb1(vec3(-1, 0, 0));
    rigidbodies.push_back(rb1);

    Rigidbody rb2(vec3(1, 1, 0), vec3(1, 2, 1));
    rb2.rotation = glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(45.0f), glm::radians(10.0f)));
    rb2.velocity = vec3(-1.0f, 0, 0);
    rigidbodies.push_back(rb2);
}

void Scene3::simulateStep()
{
    update(0.01f);
}
