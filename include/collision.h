#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>

#include <vector>

namespace Collision {

struct AABB2D {
    // Axis-Aligned Bounding Box
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

struct OBB2D {
    // Oriented Bounding Box
    glm::vec2 center;
    float halfLength;
    float halfWidth;
    float angleRadians;
};

bool intersects(const OBB2D& obb, const AABB2D& aabb, float epsilon);
bool intersectsAny(const OBB2D& obb, const std::vector<AABB2D>& aabbs, float epsilon);

}  // namespace Collision

#endif