#include "object_meshes.h"

#include <glm/ext/scalar_constants.hpp>

#include <cmath>
#include <vector>

#include "constants.h"

namespace ObjectMeshes {

Mesh createTrack() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // spine based on halfpoint between outer and inner points on ellipse
    float halfWidth = TRACK_WIDTH / 2.0f;
    float spineRx = TRACK_RADIUS_X - halfWidth;
    float spineRz = TRACK_RADIUS_Z - halfWidth;

    for (int i = 0; i <= TRACK_SEGMENTS; ++i) {
        float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(TRACK_SEGMENTS);
        
        // point on the central spine
        float px = spineRx * std::cos(theta);
        float pz = spineRz * std::sin(theta);

        // tangent vector
        float tx = -spineRx * std::sin(theta);
        float tz =  spineRz * std::cos(theta);

        // normal vector
        // swap X and Z, and negate one of them to rotate 90 degrees
        glm::vec2 normal = glm::normalize(glm::vec2(tz, -tx));

        // extrude the inner and outer points along the normal
        float outerX = px + normal.x * halfWidth;
        float outerZ = pz + normal.y * halfWidth;

        float innerX = px - normal.x * halfWidth;
        float innerZ = pz - normal.y * halfWidth;

        float uCoord = static_cast<float>(i);

        // inner vertex
        vertices.push_back({
            {innerX, 0.0f, innerZ},
            {0.0f, 1.0f, 0.0f},
            {uCoord, 0.0f}
        });

        // outer vertex
        vertices.push_back({
            {outerX, 0.0f, outerZ},
            {0.0f, 1.0f, 0.0f},
            {uCoord, 1.0f}
        });
    }

    // indices to connect vertices into triangles
    for (int i = 0; i < TRACK_SEGMENTS; ++i) {
        int startIdx = i * 2;

        // first triangle of the quad (inner, outer, next inner)
        indices.push_back(startIdx);
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 2);

        // second triangle of the quad (outer, next outer, next inner)
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 3);
        indices.push_back(startIdx + 2);
    }

    unsigned int trackTexture = 0;
    return Mesh(vertices, indices, trackTexture);
}

Mesh createGround() {
    float halfSize = WORLD_SIZE / 2.0f;

    std::vector<Vertex> vertices = {
        {{-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, WORLD_SIZE}},
        {{halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {WORLD_SIZE, WORLD_SIZE}},
        {{halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {WORLD_SIZE, 0.0f}},
        {{-halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    unsigned int groundTexture = 0;
    return Mesh(vertices, indices, groundTexture);
}

}  // namespace ObjectMeshes
