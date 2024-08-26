#pragma once
#include "ResourceManager.h"
#include <vector>
#include <utility>
#include <map>
struct Triangle
{
    uint16_t vertex[3];
};
using VertexNormalList = std::vector<ResourceManager::PrimitiveVertexAttributes>;
using TriangleList = std::vector<Triangle>;
using Lookup = std::map<std::pair<uint16_t, uint16_t>, uint16_t>;
using IndexedMesh = std::pair<VertexNormalList, TriangleList>;

namespace cube
{
    // positions and normals
    const VertexNormalList vertices = {
        {{-0.5, -0.5, -0.5}, {0.0, 0.0, -1.0}},
        {{0.5, -0.5, -0.5}, {0.0, 0.0, -1.0}},
        {{0.5, 0.5, -0.5}, {0.0, 0.0, -1.0}},
        {{-0.5, 0.5, -0.5}, {0.0, 0.0, -1.0}},
        {{-0.5, -0.5, -0.5}, {-1.0, 0.0, 0.0}},
        {{-0.5, 0.5, -0.5}, {-1.0, 0.0, 0.0}},
        {{-0.5, 0.5, 0.5}, {-1.0, 0.0, 0.0}},
        {{-0.5, -0.5, 0.5}, {-1.0, 0.0, 0.0}},
        {{0.5, -0.5, -0.5}, {1.0, 0.0, 0.0}},
        {{0.5, 0.5, -0.5}, {1.0, 0.0, 0.0}},
        {{0.5, 0.5, 0.5}, {1.0, 0.0, 0.0}},
        {{0.5, -0.5, 0.5}, {1.0, 0.0, 0.0}},
        {{-0.5, -0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{0.5, -0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{-0.5, 0.5, 0.5}, {0.0, 0.0, 1.0}},
        {{-0.5, 0.5, -0.5}, {0.0, 1.0, 0.0}},
        {{0.5, 0.5, -0.5}, {0.0, 1.0, 0.0}},
        {{0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
        {{-0.5, 0.5, 0.5}, {0.0, 1.0, 0.0}},
        {{-0.5, -0.5, -0.5}, {0.0, -1.0, 0.0}},
        {{0.5, -0.5, -0.5}, {0.0, -1.0, 0.0}},
        {{0.5, -0.5, 0.5}, {0.0, -1.0, 0.0}},
        {{-0.5, -0.5, 0.5}, {0.0, -1.0, 0.0}},
    };
    const TriangleList triangles = {
        {0, 2, 1},
        {0, 3, 2},
        {4, 6, 5},
        {4, 7, 6},
        {8, 9, 10},
        {8, 10, 11},
        {12, 13, 14},
        {12, 14, 15},
        {16, 18, 17},
        {16, 19, 18},
        {20, 21, 22},
        {20, 22, 23},
    };
}
namespace quad
{
    const VertexNormalList vertices = {
        {{-0.5, -0.5, 0}, {0, 0, 1}},
        {{0.5, -0.5, 0}, {0, 0, 1}},
        {{0.5, 0.5, 0}, {0, 0, 1}},
        {{-0.5, 0.5, 0}, {0, 0, 1}},
    };
    const TriangleList triangles = {
        {0, 1, 2},
        {0, 2, 3},
        {0, 2, 1},
        {0, 3, 2},
    };
}

// ---------------------------------------------------------------------------------------------------
// the icosphere code is adapted from https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/ |
// ---------------------------------------------------------------------------------------------------

namespace icosphere
{
    const float X = .525731112119133606f;
    const float Z = .850650808352039932f;
    const float N = 0.f;

    static const VertexNormalList vertices =
        {
            {{-X, N, Z}, {-X, N, Z}},
            {{X, N, Z}, {X, N, Z}},
            {{-X, N, -Z}, {-X, N, -Z}},
            {{X, N, -Z}, {X, N, -Z}},
            {{N, Z, X}, {N, Z, X}},
            {{N, Z, -X}, {N, Z, -X}},
            {{N, -Z, X}, {N, -Z, X}},
            {{N, -Z, -X}, {N, -Z, -X}},
            {{Z, X, N}, {Z, X, N}},
            {{-Z, X, N}, {-Z, X, N}},
            {{Z, -X, N}, {Z, -X, N}},
            {{-Z, -X, N}, {-Z, -X, N}},
    };

    static const TriangleList triangles =
        {
            {0, 1, 4},
            {0, 4, 9},
            {9, 4, 5},
            {4, 8, 5},
            {4, 1, 8},
            {8, 1, 10},
            {8, 10, 3},
            {5, 8, 3},
            {5, 3, 2},
            {2, 3, 7},
            {7, 3, 10},
            {7, 10, 6},
            {7, 6, 11},
            {11, 6, 0},
            {0, 6, 1},
            {6, 10, 1},
            {9, 11, 0},
            {9, 2, 11},
            {9, 5, 2},
            {7, 11, 2}};
}

uint16_t vertex_for_edge(Lookup &lookup,
                         VertexNormalList &vertices, uint16_t first, uint16_t second);

TriangleList subdivide(VertexNormalList &vertices,
                       TriangleList triangles);

IndexedMesh make_icosphere(int subdivisions);