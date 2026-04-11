#ifndef OBJECT_RENDERER_H
#define OBJECT_RENDERER_H

#include <glm/glm.hpp>

#include "mesh.h"

namespace ObjectRenderer {

struct Appearance {
    glm::vec3 baseColor;
    float metallic;
    float roughness;
    float opacity;
};

struct CarAppearance {
    Appearance frame;
    Appearance windows;
    Appearance wheels;
};

CarAppearance defaultCarAppearance();

void drawFloor(unsigned int shaderProgram,
                    const glm::mat4& projection,
                    const glm::mat4& view,
                    Mesh& track,
                    Mesh& ground);

void drawCar(unsigned int shaderProgram,
             const glm::mat4& projection,
             const glm::mat4& view,
             Mesh& carFrame,
             Mesh& carWindows,
             Mesh& carWheels,
             const glm::mat4& modelTransform,
             const CarAppearance& appearance);

}  // namespace ObjectRenderer

#endif