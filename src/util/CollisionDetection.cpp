#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <util/CollisionInfo.h>
#include <util/CollisionDetection.h>
#include <glm/gtx/string_cast.hpp>

// tool data structures/functions called by the collision detection method, you can ignore the details here
namespace collisionTools
{
    using vec3 = glm::vec3;
    using mat4 = glm::mat4;
    using vec4 = glm::vec4;

    vec3 getVectorConnnectingCenters(const mat4 &worldFromObj_A, const mat4 &worldFromObj_B)
    {

        const vec3 worldCenter_A = worldFromObj_A * vec4(0, 0, 0, 1);
        const vec3 worldCenter_B = worldFromObj_B * vec4(0, 0, 0, 1);
        return worldCenter_B - worldCenter_A;
    }

    // Get Corners
    std::vector<vec3> getCorners(const mat4 &worldFromObj)
    {
        const vec3 worldCenter = worldFromObj * vec4(0, 0, 0, 1);
        vec3 worldEdges[3];
        for (size_t i = 0; i < 3; ++i)
        {
            vec3 objEdge = vec3(0.0);
            objEdge[i] = 0.5f;
            worldEdges[i] = worldFromObj * vec4(objEdge, 0);
        }
        std::vector<vec3> results;
        results.push_back(worldCenter - worldEdges[0] - worldEdges[1] - worldEdges[2]);
        results.push_back(worldCenter + worldEdges[0] - worldEdges[1] - worldEdges[2]);
        results.push_back(worldCenter - worldEdges[0] + worldEdges[1] - worldEdges[2]);
        results.push_back(worldCenter + worldEdges[0] + worldEdges[1] - worldEdges[2]); // this +,+,-
        results.push_back(worldCenter - worldEdges[0] - worldEdges[1] + worldEdges[2]);
        results.push_back(worldCenter + worldEdges[0] - worldEdges[1] + worldEdges[2]); // this +,-,+
        results.push_back(worldCenter - worldEdges[0] + worldEdges[1] + worldEdges[2]); // this -,+,+
        results.push_back(worldCenter + worldEdges[0] + worldEdges[1] + worldEdges[2]); // this +,+,+
        return results;
    }

    // Get Rigid Box Size
    vec3 getBoxSize(const mat4 &worldFromObj)
    {
        vec3 size = vec3(0.0);
        vec3 edges[3];
        for (size_t i = 0; i < 3; ++i)
        {
            vec3 objEdge = vec3(0.0);
            objEdge[i] = 0.5f;
            edges[i] = worldFromObj * vec4(objEdge, 0);
            size[i] = 2.0f * glm::length(edges[i]);
        }
        return size;
    }

    // Get the Normal to the faces
    std::vector<vec3> getAxisNormalToFaces(const mat4 &worldFromObj)
    {
        std::vector<vec3> edges;
        vec4 objX = vec4(1, 0, 0, 0);
        vec4 objY = vec4(0, 1, 0, 0);
        vec4 objZ = vec4(0, 0, 1, 0);
        vec3 edge1 = glm::normalize(worldFromObj * objX);
        vec3 edge2 = glm::normalize(worldFromObj * objY);
        vec3 edge3 = glm::normalize(worldFromObj * objZ);
        std::vector<vec3> results;
        edges.push_back(edge1);
        edges.push_back(edge2);
        edges.push_back(edge3);
        return edges;
    }

    // Get the pair of edges
    std::vector<vec3> getPairOfEdges(const mat4 &worldFromObj_A, const mat4 &worldFromObj_B)
    {
        std::vector<vec3> worldEdges1 = getAxisNormalToFaces(worldFromObj_A);
        std::vector<vec3> worldEdges2 = getAxisNormalToFaces(worldFromObj_B);

        std::vector<vec3> results;
        for (int i = 0; i < worldEdges1.size(); i++)
        {
            for (int j = 0; j < worldEdges2.size(); j++)
            {
                vec3 vector = glm::cross(worldEdges1[i], worldEdges2[j]);
                if (glm::length(vector) > 0)
                    results.push_back(glm::normalize(vector));
            }
        }
        return results;
    }

    // project a shape on an axis
    Projection project(const mat4 &worldFromObj, vec3 axis)
    {
        // Get corners
        std::vector<vec3> worldCorners = getCorners(worldFromObj);
        float min = glm::dot(worldCorners[0], axis);
        float max = min;
        for (int i = 1; i < worldCorners.size(); i++)
        {
            float p = glm::dot(worldCorners[i], axis);
            if (p < min)
            {
                min = p;
            }
            else if (p > max)
            {
                max = p;
            }
        }
        Projection p;
        p.max = max;
        p.min = min;
        return p;
    }

    bool overlap(Projection p1, Projection p2)
    {
        return !((p1.max > p2.max && p1.min > p2.max) || (p2.max > p1.max && p2.min > p1.max));
    }

    float getOverlap(Projection p1, Projection p2)
    {
        return glm::min(p1.max, p2.max) - glm::max(p1.min, p2.min);
    }

    static vec3 contactPoint(
        const vec3 &pOne,
        const vec3 &dOne,
        float oneSize,
        const vec3 &pTwo,
        const vec3 &dTwo,
        float twoSize,

        // If this is true, and the contact point is outside
        // the edge (in the case of an edge-face contact) then
        // we use one's midpoint, otherwise we use two's.
        bool useOne)
    {
        vec3 cOne, cTwo;

        float smOne = glm::length2(dOne);
        float smTwo = glm::length2(dTwo);
        float dpOneTwo = glm::dot(dTwo, dOne);

        vec3 toSt = pOne - pTwo;
        float dpStaOne = glm::dot(dOne, toSt);
        float dpStaTwo = glm::dot(dTwo, toSt);

        float denom = smOne * smTwo - dpOneTwo * dpOneTwo;

        // Zero denominator indicates parrallel lines
        if (abs(denom) < 0.0001f)
        {
            return useOne ? pOne : pTwo;
        }

        float mua = (dpOneTwo * dpStaTwo - smTwo * dpStaOne) / denom;
        float mub = (smOne * dpStaTwo - dpOneTwo * dpStaOne) / denom;

        // If either of the edges has the nearest point out
        // of bounds, then the edges aren't crossed, we have
        // an edge-face contact. Our point is on the edge, which
        // we know from the useOne parameter.
        if (mua > oneSize ||
            mua < -oneSize ||
            mub > twoSize ||
            mub < -twoSize)
        {
            return useOne ? pOne : pTwo;
        }
        else
        {
            cOne = pOne + dOne * mua;
            cTwo = pTwo + dTwo * mub;

            return cOne * 0.5f + cTwo * 0.5f;
        }
    }

    vec3 handleVertexToface(const mat4 &worldFromObj, const vec3 &toCenter)
    {
        std::vector<vec3> corners = getCorners(worldFromObj);
        float min = 1000;
        vec3 vertex;
        for (int i = 0; i < corners.size(); i++)
        {
            float value = glm::dot(corners[i], toCenter);
            if (value < min)
            {
                vertex = corners[i];
                min = value;
            }
        }

        return vertex;
    }

    CollisionInfo checkCollisionSATHelper(const mat4 &worldFromObj_A, const mat4 &worldFromObj_B, vec3 size_A, vec3 size_B)
    {
        CollisionInfo info;
        info.isColliding = false;
        vec3 collisionPoint = vec3(0.0);
        float smallOverlap = 10000.0f;
        vec3 axis;
        int index;
        int fromWhere = -1;
        bool bestSingleAxis = false;
        vec3 toCenter = getVectorConnnectingCenters(worldFromObj_A, worldFromObj_B);
        std::vector<vec3> axes1 = getAxisNormalToFaces(worldFromObj_A);
        std::vector<vec3> axes2 = getAxisNormalToFaces(worldFromObj_B);
        std::vector<vec3> axes3 = getPairOfEdges(worldFromObj_A, worldFromObj_B);
        // loop over the axes1
        for (int i = 0; i < axes1.size(); i++)
        {
            // project both shapes onto the axis
            Projection p1 = project(worldFromObj_A, axes1[i]);
            Projection p2 = project(worldFromObj_B, axes1[i]);
            // do the projections overlap?
            if (!overlap(p1, p2))
            {
                // then we can guarantee that the shapes do not overlap
                return info;
            }
            else
            {
                // get the overlap
                float o = getOverlap(p1, p2);
                // check for minimum
                if (o < smallOverlap)
                {
                    // then set this one as the smallest
                    smallOverlap = o;
                    axis = axes1[i];
                    index = i;
                    fromWhere = 0;
                }
            }
        }
        // loop over the axes2
        for (int i = 0; i < axes2.size(); i++)
        {
            // project both shapes onto the axis
            Projection p1 = project(worldFromObj_A, axes2[i]);
            Projection p2 = project(worldFromObj_B, axes2[i]);
            // do the projections overlap?
            if (!overlap(p1, p2))
            {
                // then we can guarantee that the shapes do not overlap
                return info;
            }
            else
            {
                // get the overlap
                float o = getOverlap(p1, p2);
                // check for minimum
                if (o < smallOverlap)
                {
                    // then set this one as the smallest
                    smallOverlap = o;
                    axis = axes2[i];
                    index = i;
                    fromWhere = 1;
                    bestSingleAxis = true;
                }
            }
        }
        int whichEdges = 0;
        // loop over the axes3
        for (int i = 0; i < axes3.size(); i++)
        {
            // project both shapes onto the axis
            Projection p1 = project(worldFromObj_A, axes3[i]);
            Projection p2 = project(worldFromObj_B, axes3[i]);
            // do the projections overlap?
            if (!overlap(p1, p2))
            {
                // then we can guarantee that the shapes do not overlap
                return info;
            }
            else
            {
                // get the overlap
                float o = getOverlap(p1, p2);
                // check for minimum
                if (o < smallOverlap)
                {
                    // then set this one as the smallest
                    smallOverlap = o;
                    axis = axes3[i];
                    index = i;
                    whichEdges = i;
                    fromWhere = 2;
                }
            }
        }
        // if we get here then we know that every axis had overlap on it
        // so we can guarantee an intersection
        vec3 normal;
        switch (fromWhere)
        {
        case 0:
        {
            normal = axis;
            if (glm::dot(axis, toCenter) <= 0)
            {
                normal = -normal;
            }
            collisionPoint = handleVertexToface(worldFromObj_B, toCenter);
        }
        break;
        case 1:
        {
            normal = axis;
            if (glm::dot(axis, toCenter) <= 0)
            {
                normal = -normal;
            }
            collisionPoint = handleVertexToface(worldFromObj_A, toCenter * -1.0f);
        }
        break;
        case 2:
        {
            vec3 axis = glm::normalize(glm::cross(axes1[whichEdges / 3], axes2[whichEdges % 3]));
            normal = axis;
            if (glm::dot(axis, toCenter) <= 0)
            {
                normal = -normal;
            }
            vec4 ptOnOneEdge = vec4(0.5, 0.5, 0.5, 1);
            vec4 ptOnTwoEdge = vec4(0.5, 0.5, 0.5, 1);

            for (int i = 0; i < 3; i++)
            {
                if (i == whichEdges / 3)
                    ptOnOneEdge[i] = 0;
                else if (glm::dot(axes1[i], normal) < 0)
                    ptOnOneEdge[i] = -ptOnOneEdge[i];

                if (i == whichEdges % 3)
                    ptOnTwoEdge[i] = 0;
                else if (glm::dot(axes2[i], normal) > 0)
                    ptOnTwoEdge[i] = -ptOnTwoEdge[i];
            }
            ptOnOneEdge = worldFromObj_A * ptOnOneEdge;
            ptOnTwoEdge = worldFromObj_B * ptOnTwoEdge;
            collisionPoint = contactPoint(ptOnOneEdge,
                                          axes1[whichEdges / 3],
                                          size_A[whichEdges / 3],
                                          ptOnTwoEdge,
                                          axes2[whichEdges % 3],
                                          size_B[whichEdges % 3],
                                          bestSingleAxis);
        }
        break;
        }

        info.isColliding = true;
        info.collisionPointWorld = collisionPoint;
        info.depth = smallOverlap;
        info.normalWorld = -normal;
        return info;
    }

    CollisionInfo checkCollisionSAT(glm::mat4 &worldFromObj_A, glm::mat4 &worldFromObj_B)
    {
        using namespace collisionTools;
        vec3 calSizeA = getBoxSize(worldFromObj_A);
        vec3 calSizeB = getBoxSize(worldFromObj_B);

        return checkCollisionSATHelper(worldFromObj_A, worldFromObj_B, calSizeA, calSizeB);
    }

    // example of using the checkCollisionSAT function
    void testCheckCollision(int caseid)
    {

        if (caseid == 1)
        {                                                                            // simple examples, suppose that boxes A and B are cubes and have no rotation
            glm::mat4 AM = glm::translate(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0)); // box A at (1.0,1.0,1.0)
            glm::mat4 BM = glm::translate(glm::mat4(1.0), glm::vec3(2.0, 2.0, 2.0)); // box B at (2.0,2.0,2.0)

            // check for collision
            CollisionInfo simpletest = checkCollisionSAT(AM, BM); // should find out a collision here
            if (!simpletest.isColliding)
                std::cout << "No Collision" << std::endl;
            else
            {
                std::cout << "collision detected at normal: " << simpletest.normalWorld.x << ", " << simpletest.normalWorld.y << ", " << simpletest.normalWorld.z << std::endl;
                std::cout << "collision point : " << simpletest.collisionPointWorld.x << ", " << simpletest.collisionPointWorld.y << ", " << simpletest.collisionPointWorld.z << std::endl;
            }
            // case 1 result:
            // collision detected at normal: -1.000000, -0.000000, -0.000000
            // collision point : 1.500000, 1.500000, 1.500000
            // Box A should be pushed to the left
        }
        else if (caseid == 2)
        { // case 2, collide at a corner of Box B:
            mat4 scale_A = glm::scale(mat4(1.0), vec3(9.0, 2.0, 3.0));
            mat4 translate_A = glm::translate(mat4(1.0), vec3(0.2, 5.0, 1.0));
            mat4 AM = translate_A * scale_A;

            float s = 5.656855f;
            mat4 scale_B = glm::scale(mat4(1.0), vec3(s, s, 2.0));
            mat4 rotate_B = glm::rotate(mat4(1.0), glm::radians(45.0f), vec3(0, 0, 1));
            mat4 BM = rotate_B * scale_B;

            // check for collision
            CollisionInfo simpletest = checkCollisionSAT(AM, BM); // should find out a collision here

            if (!simpletest.isColliding)
                std::cout << "No Collision" << std::endl;
            else
            {
                std::cout << "collision detected at normal: " << simpletest.normalWorld.x << ", " << simpletest.normalWorld.y << ", " << simpletest.normalWorld.z << std::endl;
                std::cout << "collision point : " << simpletest.collisionPointWorld.x << ", " << simpletest.collisionPointWorld.y << ", " << simpletest.collisionPointWorld.z << std::endl;
            }
            // case 2 result:
            // collision detected at normal : 0.000000, 1.000000, 0.000000
            // collision point : 0.000000, 4.000000, 1.000000
        }
        else if (caseid == 3)
        { // case 3, collide at a corner of Box A:

            mat4 scale_A = glm::scale(mat4(1.0), vec3(2.829f, 2.829f, 2.0f));
            mat4 rotate_A = glm::rotate(mat4(1.0), glm::radians(45.0f), vec3(0, 0, 1));
            mat4 translate_A = glm::translate(mat4(1.0), vec3(-2.0, 0.0, 1.0));
            mat4 AM = translate_A * rotate_A * scale_A;

            mat4 scale_B = glm::scale(mat4(1.0), vec3(9.0f, 2.0f, 4.0f));
            mat4 rotate_B = glm::rotate(mat4(1.0), glm::radians(90.0f), vec3(0, 0, 1));
            mat4 translate_B = glm::translate(mat4(1.0), vec3(1.0, 0.5, 0.0));
            mat4 BM = translate_B * rotate_B * scale_B;

            // check for collision
            CollisionInfo simpletest = checkCollisionSAT(AM, BM); // should find out a collision here

            if (!simpletest.isColliding)
                std::cout << "No Collision" << std::endl;
            else
            {
                std::cout << "collision detected at normal: " << simpletest.normalWorld.x << ", " << simpletest.normalWorld.y << ", " << simpletest.normalWorld.z << std::endl;
                std::cout << "collision point : " << simpletest.collisionPointWorld.x << ", " << simpletest.collisionPointWorld.y << ", " << simpletest.collisionPointWorld.z << std::endl;
            }
            // case 3 result:
            // collision detected at normal: -1.000000, 0.000000, -0.000000
            // collision point : 0.000405, 0.000000, 0.000000
        }
    }
}