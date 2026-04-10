#include "object_renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace ObjectRenderer {

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

}  // namespace ObjectRenderer
