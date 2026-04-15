#include "collision.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Collision {
namespace {

glm::vec2 forwardAxis(float angleRadians) {
    return glm::normalize(glm::vec2(std::cos(angleRadians), -std::sin(angleRadians)));
}

glm::vec2 rightAxis(const glm::vec2& forward) {
    return glm::vec2(-forward.y, forward.x);
}

void projectAABB(const AABB2D& aabb, const glm::vec2& axis, float& outMin, float& outMax) {
    const std::array<glm::vec2, 4> corners = {
        glm::vec2(aabb.minX, aabb.minZ),
        glm::vec2(aabb.maxX, aabb.minZ),
        glm::vec2(aabb.maxX, aabb.maxZ),
        glm::vec2(aabb.minX, aabb.maxZ),
    };

    outMin = std::numeric_limits<float>::max();
    outMax = -std::numeric_limits<float>::max();

    for (const glm::vec2& corner : corners) {
        const float projection = glm::dot(corner, axis);
        outMin = std::min(outMin, projection);
        outMax = std::max(outMax, projection);
    }
}

void projectOBB(const OBB2D& obb, const glm::vec2& axis, float& outMin, float& outMax) {
    const glm::vec2 forward = forwardAxis(obb.angleRadians);
    const glm::vec2 right = rightAxis(forward);

    const float centerProjection = glm::dot(obb.center, axis);
    const float radius = (std::abs(glm::dot(forward, axis)) * obb.halfLength)
                       + (std::abs(glm::dot(right, axis)) * obb.halfWidth);

    outMin = centerProjection - radius;
    outMax = centerProjection + radius;
}

bool hasSeparatingAxis(const OBB2D& obb, const AABB2D& aabb, const glm::vec2& rawAxis, float epsilon) {
    const float axisLengthSquared = glm::dot(rawAxis, rawAxis);
    if (axisLengthSquared < 1e-8f) {
        return false;
    }

    const glm::vec2 axis = glm::normalize(rawAxis);

    float obbMin = 0.0f;
    float obbMax = 0.0f;
    float aabbMin = 0.0f;
    float aabbMax = 0.0f;

    projectOBB(obb, axis, obbMin, obbMax);
    projectAABB(aabb, axis, aabbMin, aabbMax);

    return (obbMax < aabbMin - epsilon) || (aabbMax < obbMin - epsilon);
}

}  // namespace

bool intersects(const OBB2D& obb, const AABB2D& aabb, float epsilon) {
    const glm::vec2 forward = forwardAxis(obb.angleRadians);
    const glm::vec2 right = rightAxis(forward);

    const std::array<glm::vec2, 4> axes = {
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        forward,
        right,
    };

    for (const glm::vec2& axis : axes) {
        if (hasSeparatingAxis(obb, aabb, axis, epsilon)) {
            return false;
        }
    }

    return true;
}

bool intersectsAny(const OBB2D& obb, const std::vector<AABB2D>& aabbs, float epsilon) {
    for (const AABB2D& aabb : aabbs) {
        if (intersects(obb, aabb, epsilon)) {
            return true;
        }
    }

    return false;
}

}  // namespace Collision