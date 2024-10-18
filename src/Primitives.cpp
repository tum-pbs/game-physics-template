#include "Primitives.h"
#include <array>
#include <webgpu/webgpu.hpp>

// ---------------------------------------------------------------------------------------------------
// the following code is adapted from https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/ |
// ---------------------------------------------------------------------------------------------------

uint16_t vertex_for_edge(Lookup &lookup,
                         VertexNormalList &vertices, uint16_t first, uint16_t second)
{
    Lookup::key_type key(first, second);
    if (key.first > key.second)
        std::swap(key.first, key.second);

    auto inserted = lookup.insert({key, static_cast<uint16_t>(vertices.size())});
    if (inserted.second)
    {
        auto &edge0 = vertices[first].position;
        auto &edge1 = vertices[second].position;
        auto point = normalize(edge0 + edge1);
        vertices.push_back({point, point});
    }

    return inserted.first->second;
}

TriangleList subdivide(VertexNormalList &vertices,
                       TriangleList triangles)
{
    Lookup lookup;
    TriangleList result;

    for (auto &&each : triangles)
    {
        std::array<uint16_t, 3> mid;
        for (int edge = 0; edge < 3; ++edge)
        {
            mid[edge] = vertex_for_edge(lookup, vertices,
                                        each.vertex[edge], each.vertex[(edge + 1) % 3]);
        }

        result.push_back({each.vertex[0], mid[0], mid[2]});
        result.push_back({each.vertex[1], mid[1], mid[0]});
        result.push_back({each.vertex[2], mid[2], mid[1]});
        result.push_back({mid[0], mid[1], mid[2]});
    }

    return result;
}

IndexedMesh make_icosphere(int subdivisions)
{
    VertexNormalList vertices = icosphere::vertices;
    TriangleList triangles = icosphere::triangles;

    for (int i = 0; i < subdivisions; ++i)
    {
        triangles = subdivide(vertices, triangles);
    }

    return {vertices, triangles};
}