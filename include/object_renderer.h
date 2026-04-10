#ifndef OBJECT_RENDERER_H
#define OBJECT_RENDERER_H

#include <glm/glm.hpp>

#include "mesh.h"

namespace ObjectRenderer {

void drawFloor(unsigned int shaderProgram,
                     const glm::mat4& projection,
                     const glm::mat4& view,
                     Mesh& track,
                     Mesh& ground);

}  // namespace ObjectRenderer

#endif