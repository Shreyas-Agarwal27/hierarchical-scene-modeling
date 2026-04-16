#ifndef BUILDING_MODELS_H
#define BUILDING_MODELS_H

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "mesh.h"

namespace BuildingModels {

struct Instance {
    glm::vec3 position;
    std::size_t prototypeIndex;
    float scale;
    float roofY;
    float halfWidth;
    float halfDepth;
};

struct LightRigConfig {
    glm::vec3 basePosition;
    glm::vec3 baseDirection;
    glm::vec3 color;
    float swingSpeed;
};

struct Scene {
    struct Prototype {
        Mesh mesh;
        glm::vec3 localMin;
        glm::vec3 localMax;
        float scale;
    };

    std::vector<Prototype> prototypes;
    std::vector<Instance> instances;
};

Scene loadTowerScene(unsigned int colormapTextureID);

const Mesh& meshFor(const Scene& scene, const Instance& instance);
Mesh& meshFor(Scene& scene, const Instance& instance);

glm::mat4 modelTransformFor(const Scene& scene, const Instance& instance);

std::vector<LightRigConfig> createLightRigConfigs(const Scene& scene);

float roadFacingSignX(const Instance& instance);
float roadFacingYawRadians(const Instance& instance);
glm::vec3 roofCenter(const Instance& instance);

void cleanup(Scene& scene);

}  // namespace BuildingModels

#endif