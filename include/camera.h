#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMode {
    SKY_VIEW,
    CAR_VIEW,
    GROUND_VIEW,
    LIGHTSOURCE_VIEW,
    HELICOPTER_CAM
};

class Camera {
public:
    CameraMode currentMode = CameraMode::SKY_VIEW;
    
    // for ground view left/right looking
    float groundViewYaw = 0.0f; 

    // changes the current mode
    void setMode(CameraMode mode) {
        currentMode = mode;
    }

    // generates the view matrix based on the current mode and world states
    glm::mat4 getViewMatrix(
        glm::vec3 carPosition, float carAngle,
        glm::vec3 groundCamPos, 
        glm::vec3 lightPos, glm::vec3 lightDir
    );
};

#endif