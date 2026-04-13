// camera.cpp
#include "camera.h"
#include "constants.h"
#include <cmath>
#include <algorithm>

glm::mat4 Camera::getViewMatrix(
                    glm::vec3 carPosition, float carAngle,
                    glm::vec3 groundCamPos, 
                    glm::vec3 lightPos, float lightAngle) {

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::vec3 carForward(std::cos(carAngle), 0.0f, -std::sin(carAngle));

    switch (currentMode) {
        case CameraMode::SKY_VIEW: {
            return glm::lookAt(glm::vec3(0.0f, CAMERA_SKY_HEIGHT, 0.0f), 
                               glm::vec3(0.0f, 0.0f, 0.0f), 
                               glm::vec3(0.0f, 0.0f, -1.0f));
        }
        case CameraMode::CAR_VIEW: {
            // top of the cabin
            float localRoofY = CAR_CABIN_OFFSET_Y + (CAR_CABIN_HEIGHT / 2.0f);
            // front edge of the cabin
            float localCabinFrontX = CAR_CABIN_OFFSET_X + (CAR_CABIN_LENGTH / 2.0f);
            
            float worldRoofY = localRoofY * CAR_SCALE;
            float worldCabinFrontX = localCabinFrontX * CAR_SCALE;
            
            // position the camera on the roof, push it up slightly so it doesn't clip the roof mesh
            glm::vec3 eye = carPosition 
                          + glm::vec3(0.0f, worldRoofY + 0.5f, 0.0f) 
                          + (carForward * worldCabinFrontX);
                          
            // look ahead of the car and slightly down
            glm::vec3 target = eye + (carForward * 10.0f) - glm::vec3(0.0f, 2.5f, 0.0f);
            
            return glm::lookAt(eye, target, up);
        }
        case CameraMode::HELICOPTER_CAM: {
            // behind and above the car 
            glm::vec3 eye = carPosition - (carForward * CAMERA_HELICOPTER_DISTANCE) + glm::vec3(0.0f, CAMERA_HELICOPTER_HEIGHT, 0.0f);
            return glm::lookAt(eye, carPosition, up);
        }
        case CameraMode::GROUND_VIEW: {
            // front of a building, looking left/right 
            // clamp yaw to +/- 30 degrees
            float yaw = std::clamp(groundViewYaw, -glm::radians(30.0f), glm::radians(30.0f));
            glm::vec3 baseLookDir = glm::normalize(glm::vec3(-groundCamPos.x, 0.0f, 0.0f));
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), yaw, up);
            glm::vec3 lookDir = glm::vec3(rotation * glm::vec4(baseLookDir, 0.0f));
            return glm::lookAt(groundCamPos, groundCamPos + lookDir, up);
        }
        case CameraMode::LIGHTSOURCE_VIEW: {
            glm::vec3 lightForward(std::sin(lightAngle), -0.5f, -std::cos(lightAngle));
            return glm::lookAt(lightPos, lightPos + lightForward, up);
        }
    }
    return glm::mat4(1.0f);
}