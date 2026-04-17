#include "object_meshes.h"

#include <glm/ext/scalar_constants.hpp>

#include <cmath>
#include <vector>

#include "constants.h"

namespace ObjectMeshes {

Mesh createTrack(unsigned int textureID) {
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

    return Mesh(vertices, indices, textureID);
}

Mesh createGround(unsigned int textureID) {
    float halfSize = WORLD_SIZE / 2.0f;
    const float uvScale = WORLD_SIZE / 10.0f;

    std::vector<Vertex> vertices = {
        {{-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, uvScale}},
        {{halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {uvScale, uvScale}},
        {{halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {uvScale, 0.0f}},
        {{-halfSize, 0.0f, halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    return Mesh(vertices, indices, textureID);
}

namespace {
void addBox(std::vector<Vertex>& vertices,
            std::vector<unsigned int>& indices,
            const glm::vec3& center,
            const glm::vec3& size) {
    float hx = size.x / 2.0f;
    float hy = size.y / 2.0f;
    float hz = size.z / 2.0f;

    glm::vec3 p[8] = {
        center + glm::vec3(-hx, -hy, -hz),
        center + glm::vec3(hx, -hy, -hz),
        center + glm::vec3(hx, hy, -hz),
        center + glm::vec3(-hx, hy, -hz),
        center + glm::vec3(-hx, -hy, hz),
        center + glm::vec3(hx, -hy, hz),
        center + glm::vec3(hx, hy, hz),
        center + glm::vec3(-hx, hy, hz),
    };

    constexpr float uvScale = 0.25f;

    auto addFace = [&](int i0,
                       int i1,
                       int i2,
                       int i3,
                       const glm::vec3& normal,
                       float uSize,
                       float vSize) {
        unsigned int startIdx = static_cast<unsigned int>(vertices.size());
        const float uRepeat = uSize * uvScale;
        const float vRepeat = vSize * uvScale;

        vertices.push_back({p[i0], normal, {0.0f, 0.0f}});
        vertices.push_back({p[i1], normal, {uRepeat, 0.0f}});
        vertices.push_back({p[i2], normal, {uRepeat, vRepeat}});
        vertices.push_back({p[i3], normal, {0.0f, vRepeat}});

        indices.push_back(startIdx);
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 2);
        indices.push_back(startIdx);
        indices.push_back(startIdx + 2);
        indices.push_back(startIdx + 3);
    };

    addFace(0, 4, 5, 1, glm::vec3(0, -1, 0), size.x, size.z);
    addFace(3, 2, 6, 7, glm::vec3(0, 1, 0), size.x, size.z);
    addFace(0, 1, 2, 3, glm::vec3(0, 0, -1), size.x, size.y);
    addFace(5, 4, 7, 6, glm::vec3(0, 0, 1), size.x, size.y);
    addFace(4, 0, 3, 7, glm::vec3(-1, 0, 0), size.z, size.y);
    addFace(1, 5, 6, 2, glm::vec3(1, 0, 0), size.z, size.y);
}

void addCylinderZ(std::vector<Vertex>& vertices,
                  std::vector<unsigned int>& indices,
                  const glm::vec3& center,
                  float radius,
                  float depth,
                  int segments) {
    if (segments < 3) {
        return;
    }

    float halfDepth = depth * 0.5f;
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        float t = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float cx = std::cos(t);
        float cy = std::sin(t);
        glm::vec3 normal = glm::normalize(glm::vec3(cx, cy, 0.0f));

        glm::vec3 frontPos = center + glm::vec3(radius * cx, radius * cy, halfDepth);
        glm::vec3 backPos = center + glm::vec3(radius * cx, radius * cy, -halfDepth);

        vertices.push_back({frontPos, normal, {static_cast<float>(i) / static_cast<float>(segments), 1.0f}});
        vertices.push_back({backPos, normal, {static_cast<float>(i) / static_cast<float>(segments), 0.0f}});
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int i0 = baseIndex + static_cast<unsigned int>(i * 2);
        unsigned int i1 = i0 + 1;
        unsigned int i2 = i0 + 2;
        unsigned int i3 = i0 + 3;

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i1);
        indices.push_back(i3);
    }

    unsigned int capStart = static_cast<unsigned int>(vertices.size());
    unsigned int frontCenter = capStart;
    vertices.push_back({center + glm::vec3(0.0f, 0.0f, halfDepth), glm::vec3(0.0f, 0.0f, 1.0f), {0.5f, 0.5f}});
    unsigned int backCenter = capStart + 1;
    vertices.push_back({center + glm::vec3(0.0f, 0.0f, -halfDepth), glm::vec3(0.0f, 0.0f, -1.0f), {0.5f, 0.5f}});

    unsigned int frontRingStart = static_cast<unsigned int>(vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float t = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float cx = std::cos(t);
        float cy = std::sin(t);
        vertices.push_back({
            center + glm::vec3(radius * cx, radius * cy, halfDepth),
            glm::vec3(0.0f, 0.0f, 1.0f),
            {0.5f + 0.5f * cx, 0.5f + 0.5f * cy}
        });
    }

    unsigned int backRingStart = static_cast<unsigned int>(vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float t = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float cx = std::cos(t);
        float cy = std::sin(t);
        vertices.push_back({
            center + glm::vec3(radius * cx, radius * cy, -halfDepth),
            glm::vec3(0.0f, 0.0f, -1.0f),
            {0.5f + 0.5f * cx, 0.5f + 0.5f * cy}
        });
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int v0 = frontRingStart + static_cast<unsigned int>(i);
        unsigned int v1 = frontRingStart + static_cast<unsigned int>(i + 1);
        indices.push_back(frontCenter);
        indices.push_back(v0);
        indices.push_back(v1);
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int v0 = backRingStart + static_cast<unsigned int>(i);
        unsigned int v1 = backRingStart + static_cast<unsigned int>(i + 1);
        indices.push_back(backCenter);
        indices.push_back(v1);
        indices.push_back(v0);
    }
}
}  // namespace

Mesh createCarFrame() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    addBox(vertices, indices, 
            glm::vec3(0.0f, CAR_FRAME_HEIGHT, 0.0f), 
            glm::vec3(CAR_FRAME_LENGTH, CAR_FRAME_HEIGHT, CAR_FRAME_WIDTH));

    addBox(vertices, indices, 
           glm::vec3(CAR_CABIN_OFFSET_X, CAR_CABIN_OFFSET_Y, CAR_CABIN_OFFSET_Z), 
           glm::vec3(CAR_CABIN_LENGTH, CAR_CABIN_HEIGHT, CAR_CABIN_WIDTH));

    return Mesh(vertices, indices, 0);
}

Mesh createCarWindows() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float frontEdge = CAR_CABIN_OFFSET_X + (CAR_CABIN_LENGTH / 2.0f);
    float rearEdge  = CAR_CABIN_OFFSET_X - (CAR_CABIN_LENGTH / 2.0f);
    float leftEdge  = CAR_CABIN_OFFSET_Z + (CAR_CABIN_WIDTH / 2.0f);
    float rightEdge = CAR_CABIN_OFFSET_Z - (CAR_CABIN_WIDTH / 2.0f);
    
    float windowY = CAR_CABIN_OFFSET_Y + 0.1f; // slightly up from cabin centre

    // windshield
    addBox(vertices, indices, 
           glm::vec3(frontEdge, windowY, CAR_CABIN_OFFSET_Z), 
           glm::vec3(CAR_WINDOW_THICKNESS, CAR_WINDOW_HEIGHT, CAR_CABIN_WIDTH + 0.2f));

    // rear window
    addBox(vertices, indices, 
           glm::vec3(rearEdge, windowY, CAR_CABIN_OFFSET_Z), 
           glm::vec3(CAR_WINDOW_THICKNESS, CAR_WINDOW_HEIGHT, CAR_CABIN_WIDTH + 0.2f));

    // left side
    addBox(vertices, indices, 
           glm::vec3(CAR_CABIN_OFFSET_X, windowY, leftEdge), 
           glm::vec3(CAR_CABIN_LENGTH + 0.2f, CAR_WINDOW_HEIGHT, CAR_WINDOW_THICKNESS));

    // right side
    addBox(vertices, indices, 
           glm::vec3(CAR_CABIN_OFFSET_X, windowY, rightEdge), 
           glm::vec3(CAR_CABIN_LENGTH + 0.2f, CAR_WINDOW_HEIGHT, CAR_WINDOW_THICKNESS));
    return Mesh(vertices, indices, 0);
}

Mesh createCarWheels() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const int wheelSegments = 24;
    const float wheelDepth = 1.8f;

    float wheelX = (CAR_FRAME_LENGTH / 2.0f) * CAR_WHEEL_X_RATIO;
    float wheelY = CAR_WHEEL_RADIUS;
    float wheelZ = (CAR_FRAME_WIDTH / 2.0f) + CAR_WHEEL_Z_PROTRUSION;

    addCylinderZ(vertices, indices, glm::vec3(wheelX, wheelY, wheelZ), CAR_WHEEL_RADIUS, wheelDepth, wheelSegments);
    addCylinderZ(vertices, indices, glm::vec3(wheelX, wheelY, -wheelZ), CAR_WHEEL_RADIUS, wheelDepth, wheelSegments);
    addCylinderZ(vertices, indices, glm::vec3(-wheelX, wheelY, wheelZ), CAR_WHEEL_RADIUS, wheelDepth, wheelSegments);
    addCylinderZ(vertices, indices, glm::vec3(-wheelX, wheelY, -wheelZ), CAR_WHEEL_RADIUS, wheelDepth, wheelSegments);

    return Mesh(vertices, indices, 0);
}

Mesh createWindmill(unsigned int textureID) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // vertical blade
    addBox(vertices, indices, 
           glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3(WINDMILL_BLADE_WIDTH, WINDMILL_RADIUS * 2.0f, WINDMILL_DEPTH));
    
    // horizontal blade
    addBox(vertices, indices, 
           glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3(WINDMILL_RADIUS * 2.0f, WINDMILL_BLADE_WIDTH, WINDMILL_DEPTH));

    return Mesh(vertices, indices, textureID);
}

Mesh createLightGimbalBase() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    addBox(vertices,
           indices,
           glm::vec3(0.0f, BUILDING_LIGHT_GIMBAL_BASE_HEIGHT * 0.5f, 0.0f),
           glm::vec3(BUILDING_LIGHT_GIMBAL_BASE_WIDTH,
                     BUILDING_LIGHT_GIMBAL_BASE_HEIGHT,
                     BUILDING_LIGHT_GIMBAL_BASE_DEPTH));

    const float postHeight = BUILDING_LIGHT_GIMBAL_PIVOT_HEIGHT - BUILDING_LIGHT_GIMBAL_BASE_HEIGHT;
    addBox(vertices,
           indices,
           glm::vec3(0.0f, BUILDING_LIGHT_GIMBAL_BASE_HEIGHT + (postHeight * 0.5f), 0.0f),
           glm::vec3(BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS,
                     postHeight,
                     BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS));

    return Mesh(vertices, indices, 0);
}

Mesh createLightGimbalHead() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float halfYokeSpan = BUILDING_LIGHT_GIMBAL_YOKE_WIDTH * 0.5f;
    const float yokeLegCenterY = -BUILDING_LIGHT_GIMBAL_YOKE_HEIGHT * 0.5f;

    // U-shaped yoke around the lamp, with origin at the yaw pivot.
    addBox(vertices,
           indices,
           glm::vec3(0.0f, yokeLegCenterY, halfYokeSpan),
           glm::vec3(BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS,
                     BUILDING_LIGHT_GIMBAL_YOKE_HEIGHT,
                     BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS));

    addBox(vertices,
           indices,
           glm::vec3(0.0f, yokeLegCenterY, -halfYokeSpan),
           glm::vec3(BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS,
                     BUILDING_LIGHT_GIMBAL_YOKE_HEIGHT,
                     BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS));

    addBox(vertices,
           indices,
           glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3(BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS,
                     BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS,
                     BUILDING_LIGHT_GIMBAL_YOKE_WIDTH));

    // Lamp body points toward +X in local space.
    addBox(vertices,
           indices,
           glm::vec3(BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_CENTER_X_FACTOR,
                     BUILDING_LIGHT_LAMP_CENTER_Y,
                     0.0f),
           glm::vec3(BUILDING_LIGHT_LAMP_DEPTH,
                     BUILDING_LIGHT_LAMP_HEIGHT,
                     BUILDING_LIGHT_LAMP_WIDTH));

    addBox(vertices,
           indices,
           glm::vec3(BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_NOZZLE_X_FACTOR,
                     BUILDING_LIGHT_LAMP_CENTER_Y,
                     0.0f),
           glm::vec3(BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_NOZZLE_DEPTH_FACTOR,
                     BUILDING_LIGHT_LAMP_HEIGHT * BUILDING_LIGHT_LAMP_NOZZLE_HEIGHT_FACTOR,
                     BUILDING_LIGHT_LAMP_WIDTH * BUILDING_LIGHT_LAMP_NOZZLE_WIDTH_FACTOR));

    return Mesh(vertices, indices, 0);
}

Mesh createUnitBox() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    addBox(vertices, indices, glm::vec3(0.0f), glm::vec3(1.0f));

    return Mesh(vertices, indices, 0);
}

Mesh createWalls(unsigned int textureID) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfSize = WORLD_SIZE / 2.0f;
    float yCenter = WALL_HEIGHT / 2.0f;

    // north wall (+Z edge)
    addBox(vertices, indices, 
           glm::vec3(0.0f, yCenter, halfSize), 
           glm::vec3(WORLD_SIZE, WALL_HEIGHT, WALL_THICKNESS));
    
    // south wall (-Z edge)
    addBox(vertices, indices, 
           glm::vec3(0.0f, yCenter, -halfSize), 
           glm::vec3(WORLD_SIZE, WALL_HEIGHT, WALL_THICKNESS));

    // east wall (+X edge)
    addBox(vertices, indices, 
           glm::vec3(halfSize, yCenter, 0.0f), 
           glm::vec3(WALL_THICKNESS, WALL_HEIGHT, WORLD_SIZE + WALL_THICKNESS)); // add thickness to prevent corner gaps

    // west wall (-X edge)
    addBox(vertices, indices, 
           glm::vec3(-halfSize, yCenter, 0.0f), 
           glm::vec3(WALL_THICKNESS, WALL_HEIGHT, WORLD_SIZE + WALL_THICKNESS));

    return Mesh(vertices, indices, textureID);
}

}  // namespace ObjectMeshes
