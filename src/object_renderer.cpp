#include "object_renderer.h"
#include "constants.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace ObjectRenderer {

namespace {

struct RendererUniformLocations {
    unsigned int program = 0;
    int projection = -1;
    int view = -1;
    int transform = -1;
    int objectColor = -1;
    int shininess = -1;
    int specularStrength = -1;
};

RendererUniformLocations gRendererUniforms;

void cacheRendererUniformLocations(unsigned int shaderProgram) {
    if (gRendererUniforms.program == shaderProgram) {
        return;
    }

    gRendererUniforms.program = shaderProgram;
    gRendererUniforms.projection = glGetUniformLocation(shaderProgram, "projection");
    gRendererUniforms.view = glGetUniformLocation(shaderProgram, "view");
    gRendererUniforms.transform = glGetUniformLocation(shaderProgram, "transform");
    gRendererUniforms.objectColor = glGetUniformLocation(shaderProgram, "objectColor");
    gRendererUniforms.shininess = glGetUniformLocation(shaderProgram, "shininess");
    gRendererUniforms.specularStrength = glGetUniformLocation(shaderProgram, "specularStrength");
}

}  // namespace

CarAppearance defaultCarAppearance() {
    return {
        {glm::vec3(0.85f, 0.10f, 0.10f), 0.9f, 0.2f, 1.0f},
        {glm::vec3(0.20f, 0.45f, 0.95f), 0.0f, 0.1f, 1.0f},
        {glm::vec3(0.05f, 0.05f, 0.05f), 0.0f, 0.8f, 1.0f},
    };
}

// draws floor (track and ground)
void drawFloor(unsigned int shaderProgram,
                    const glm::mat4& projection,
                    const glm::mat4& view,
                    Mesh& track,
                    Mesh& ground) {
    cacheRendererUniformLocations(shaderProgram);
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(gRendererUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(gRendererUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniform1f(gRendererUniforms.shininess, DIFFUSE_SHININESS);
    glUniform1f(gRendererUniforms.specularStrength, DIFFUSE_SPECULAR_STRENGTH);

    glEnable(GL_STENCIL_TEST);

    // draw track
    // setup stencil to write a 1 wherever the track is drawn
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF); // always pass the stencil test
    glStencilMask(0xFF); // enable writing to stencil buffer

    const glm::mat4 trackTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(gRendererUniforms.transform, 1, GL_FALSE, glm::value_ptr(trackTransform));

    glUniform3f(gRendererUniforms.objectColor, 1.0f, 1.0f, 1.0f);
    track.draw(shaderProgram);

    // draw ground
    // Setup stencil to only draw if the stencil value is NOT 1
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00); // reading from stencil buffer

    const glm::mat4 groundTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(gRendererUniforms.transform, 1, GL_FALSE, glm::value_ptr(groundTransform));
    glUniform3f(gRendererUniforms.objectColor, 1.0f, 1.0f, 1.0f);
    ground.draw(shaderProgram);

    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
}

void drawCar(unsigned int shaderProgram,
             const glm::mat4& projection,
             const glm::mat4& view,
             Mesh& carFrame,
             Mesh& carWindows,
             Mesh& carWheels,
             const glm::mat4& modelTransform,
             const CarAppearance& appearance) {
    cacheRendererUniformLocations(shaderProgram);
    glUseProgram(shaderProgram);
    
    glUniformMatrix4fv(gRendererUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(gRendererUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(gRendererUniforms.transform, 1, GL_FALSE, glm::value_ptr(modelTransform));

    glUniform1f(gRendererUniforms.shininess, METAL_SHININESS);
    glUniform1f(gRendererUniforms.specularStrength, METAL_SPECULAR_STRENGTH);

    glUniform3f(gRendererUniforms.objectColor,
                appearance.frame.baseColor.r,
                appearance.frame.baseColor.g,
                appearance.frame.baseColor.b);
    carFrame.draw(shaderProgram);

    glUniform3f(gRendererUniforms.objectColor,
                appearance.windows.baseColor.r,
                appearance.windows.baseColor.g,
                appearance.windows.baseColor.b);
    glUniform1f(gRendererUniforms.shininess, DIFFUSE_SHININESS);
    glUniform1f(gRendererUniforms.specularStrength, DIFFUSE_SPECULAR_STRENGTH);
    carWindows.draw(shaderProgram);

    glUniform3f(gRendererUniforms.objectColor,
                appearance.wheels.baseColor.r,
                appearance.wheels.baseColor.g,
                appearance.wheels.baseColor.b);
    glUniform1f(gRendererUniforms.shininess, DIFFUSE_SHININESS);
    glUniform1f(gRendererUniforms.specularStrength, DIFFUSE_SPECULAR_STRENGTH);
    carWheels.draw(shaderProgram);
}

void drawBuilding(unsigned int shaderProgram,
                  const glm::mat4& projection,
                  const glm::mat4& view,
                  Mesh& buildingMesh,
                  const glm::mat4& modelTransform,
                  const glm::vec3& color) {
    cacheRendererUniformLocations(shaderProgram);
    glUseProgram(shaderProgram);
    
    glUniformMatrix4fv(gRendererUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(gRendererUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(gRendererUniforms.transform, 1, GL_FALSE, glm::value_ptr(modelTransform));
    glUniform1f(gRendererUniforms.shininess, DIFFUSE_SHININESS);
    glUniform1f(gRendererUniforms.specularStrength, DIFFUSE_SPECULAR_STRENGTH);

    glUniform3f(gRendererUniforms.objectColor, color.r, color.g, color.b);

    buildingMesh.draw(shaderProgram);
}

}  // namespace ObjectRenderer
