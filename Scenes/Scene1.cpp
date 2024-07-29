#include "Scene1.h"
#include <imgui.h>
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include "CollisionDetection.h"

void Scene1::init()
{
    Rigidbody rb(
        vec3(0, 0, 0),
        vec3(1, 0.6, 0.5),
        2);
    rb.rotation = glm::angleAxis(glm::radians(90.0f), vec3(0, 0, 1));
    rb.update(0.0f);
    vec3 force = vec3(1, 1, 0);
    vec3 fwhere = vec3(0.3, 0.5, 0.25);
    rb.addForceWorld(force, fwhere);
    rigidbodies.push_back(rb);
    update(2.0f);
    rb = rigidbodies[0];

    std::cout << "new Position: " << glm::to_string(rb.position) << std::endl;
    std::cout << "new Velocity: " << glm::to_string(rb.velocity) << std::endl;
    std::cout << "new Angular Velocity: " << glm::to_string(rb.angularVelocity) << std::endl;

    vec3 xa_world = vec3(-0.3f, -0.5f, -0.25f) - rb.position;
    vec3 velocityA = rb.velocity + cross(rb.angularVelocity, xa_world);

    std::cout << "vel at P(-0.3, -0.5, -0.25), " << glm::to_string(velocityA) << std::endl;

    // new Position, x, 0, y, 0, z, 0
    // new Velocity, x, 1, y, 1, z, 0
    // new Angular V, x, -2.4, y, 4.91803, z, -1.76471
    // vel at P(-0.3, -0.5, -0.25), x, -1.11186, y, 0.929412, z, 2.67541

    collisionTools::testCheckCollision(1);
    collisionTools::testCheckCollision(2);
    collisionTools::testCheckCollision(3);
}
