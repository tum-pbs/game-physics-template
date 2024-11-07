#pragma once
#include <glm/glm.hpp>

// the return structure, with these values, you should be able to calculate the impulse
// the depth shouldn't be used in your impulse calculation, it is a redundant value
// if the normalWorld == XMVectorZero(), no collision
struct CollisionInfo
{
    bool isColliding;              // whether there is a collision point, true for yes
    glm::vec3 collisionPointWorld; // the position of the collision point in world space
    glm::vec3 normalWorld;         // the direction of the impulse to A, negative of the collision face of A
    float depth;                   // the distance of the collision point to the surface, not necessary.
};