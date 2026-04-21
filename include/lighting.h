#ifndef LIGHTING_H
#define LIGHTING_H

#include <vector>

#include <glm/glm.hpp>

#include "building_models.h"

namespace Lighting {

struct BuildingLight {
    glm::vec3 basePosition;
    glm::vec3 baseDirection;
    glm::vec3 color;

    float currentSwingAngle = 0.0f;
    float swingSpeed = 0.0f;
};

struct DayNightCycleState {
    float cycleProgress = 0.0f;
    float daylightFactor = 1.0f;
    float nightFactor = 0.0f;

    float globalAmbientStrength = 0.0f;
    float spotlightAmbientStrength = 0.0f;
    float headlightAmbientStrength = 0.0f;

    float buildingLightColorScale = 1.0f;
    float headlightColorScale = 1.0f;

    float sunAmbientStrength = 0.0f;
    float sunDiffuseStrength = 0.0f;
    float sunSpecularStrength = 0.0f;

    glm::vec3 skyColor = glm::vec3(0.0f);
    glm::vec3 sunDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 sunColor = glm::vec3(1.0f);
};

DayNightCycleState evaluateDayNightCycle(float currentTime);

std::vector<BuildingLight> createBuildingLights(const BuildingModels::Scene& scene);
float swingAngleForLight(const BuildingLight& light, float time);
glm::vec3 directionForLight(const BuildingLight& light, float time);
glm::vec3 positionForLight(const BuildingLight& light, float time);

void setupLighting(unsigned int shaderProgram,
                   std::vector<BuildingLight>& lights,
                   const BuildingModels::Scene& towerScene,
                   float currentWindmillAngle,
                   const glm::vec3& viewPos,
                   const glm::mat4& carModel,
                   bool headlightsOn,
                   float time,
                   const DayNightCycleState& dayNightState);

}  // namespace Lighting

#endif
