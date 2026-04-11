#include "object_renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace ObjectRenderer {

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
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glEnable(GL_STENCIL_TEST);

    // draw track
    // setup stencil to write a 1 wherever the track is drawn
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF); // always pass the stencil test
    glStencilMask(0xFF); // enable writing to stencil buffer

    glm::mat4 trackTransform = glm::mat4(1.0f);
    trackTransform = glm::scale(trackTransform, glm::vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(trackTransform));

    int colorLocation = glGetUniformLocation(shaderProgram, "objectColor");
    glUniform3f(colorLocation, 0.2f, 0.2f, 0.2f);
    track.draw(shaderProgram);

    // draw ground
    // Setup stencil to only draw if the stencil value is NOT 1
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00); // reading from stencil buffer

    glm::mat4 groundTransform = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(groundTransform));
    glUniform3f(colorLocation, 0.0f, 1.0f, 0.0f);
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
    glUseProgram(shaderProgram);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(modelTransform));

    int colorLocation = glGetUniformLocation(shaderProgram, "objectColor");

    glUniform3f(colorLocation,
                appearance.frame.baseColor.r,
                appearance.frame.baseColor.g,
                appearance.frame.baseColor.b);
    carFrame.draw(shaderProgram);

    glUniform3f(colorLocation,
                appearance.windows.baseColor.r,
                appearance.windows.baseColor.g,
                appearance.windows.baseColor.b);
    carWindows.draw(shaderProgram);

    glUniform3f(colorLocation,
                appearance.wheels.baseColor.r,
                appearance.wheels.baseColor.g,
                appearance.wheels.baseColor.b);
    carWheels.draw(shaderProgram);
}

}  // namespace ObjectRenderer
