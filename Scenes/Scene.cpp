#include "Scene.h"
#include <stdio.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

void Scene::onDraw(Renderer &renderer)
{
    for (int i = 0; i < rigidbodies.size(); i++)
    {
        renderer.drawCube(
            rigidbodies[i].position,
            rigidbodies[i].rotation,
            rigidbodies[i].scale,
            {1, 0, 0});
    }
}

void Scene::update(float deltaTime)
{

    for (Rigidbody &rigidBody : rigidbodies)
    {
        rigidBody.update(deltaTime);
    }
    for (size_t i = 0; i < rigidbodies.size(); ++i)
    {
        for (size_t j = i + 1; j < rigidbodies.size(); ++j)
        {
            CollisionInfo info = Rigidbody::collide(rigidbodies[i], rigidbodies[j]);
        }
    }
}
