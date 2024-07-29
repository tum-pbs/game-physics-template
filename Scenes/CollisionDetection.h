#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include "CollisionInfo.h"

// tool data structures/functions called by the collision detection method, you can ignore the details here
namespace collisionTools
{

    struct Projection
    {
        float min, max;
    };

    glm::vec3 getVectorConnnectingCenters(const glm::mat4 &worldFromObj_A, const glm::mat4 &worldFromObj_B);
    // Get Corners
    std::vector<glm::vec3> getCorners(const glm::mat4 &worldFromObj);

    // Get Rigid Box Size
    glm::vec3 getBoxSize(const glm::mat4 &worldFromObj);

    // Get the Normal to the faces
    std::vector<glm::vec3> getAxisNormalToFaces(const glm::mat4 &worldFromObj);

    // Get the pair of edges
    std::vector<glm::vec3> getPairOfEdges(const glm::mat4 &obj2World_A, const glm::mat4 &obj2World_B);

    // project a shape on an axis
    Projection project(const glm::mat4 &obj2World, glm::vec3 axis);

    bool overlap(Projection p1, Projection p2);

    float getOverlap(Projection p1, Projection p2);

    static glm::vec3 contactPoint(
        const glm::vec3 &pOne,
        const glm::vec3 &dOne,
        float oneSize,
        const glm::vec3 &pTwo,
        const glm::vec3 &dTwo,
        float twoSize,

        // If this is true, and the contact point is outside
        // the edge (in the case of an edge-face contact) then
        // we use one's midpoint, otherwise we use two's.
        bool useOne);

    glm::vec3 handleVertexToface(const glm::mat4 &obj2World, const glm::vec3 &toCenter);

    CollisionInfo checkCollisionSATHelper(const glm::mat4 &obj2World_A, const glm::mat4 &obj2World_B, glm::vec3 size_A, glm::vec3 size_B);

    /* params:
    obj2World_A, the transfer matrix from object space of A to the world space
    obj2World_B, the transfer matrix from object space of B to the world space
    */
    CollisionInfo checkCollisionSAT(glm::mat4 &obj2World_A, glm::mat4 &obj2World_B);

    // example of using the checkCollisionSAT function
    void testCheckCollision(int caseid);
}