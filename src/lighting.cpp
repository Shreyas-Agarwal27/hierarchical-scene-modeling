#include "lighting.h"

#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "constants.h"

namespace Lighting {

namespace {

struct CarHeadlightState {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
};

struct LightingUniformLocations {
    int viewPos = -1;
    int globalAmbientStrength = -1;
    int spotlightAmbientStrength = -1;
    int headlightAmbientStrength = -1;
    int sunDirection = -1;
    int sunColor = -1;
    int sunAmbientStrength = -1;
    int sunDiffuseStrength = -1;
    int sunSpecularStrength = -1;
    int numBuildingLights = -1;
    int numCarHeadlights = -1;

    std::array<int, MAX_BUILDING_LIGHTS> position{};
    std::array<int, MAX_BUILDING_LIGHTS> direction{};
    std::array<int, MAX_BUILDING_LIGHTS> color{};
    std::array<int, MAX_BUILDING_LIGHTS> cutOff{};
    std::array<int, MAX_BUILDING_LIGHTS> outerCutOff{};
    std::array<int, MAX_BUILDING_LIGHTS> constant{};
    std::array<int, MAX_BUILDING_LIGHTS> linear{};
    std::array<int, MAX_BUILDING_LIGHTS> quadratic{};

    std::array<int, MAX_CAR_HEADLIGHTS> carPosition{};
    std::array<int, MAX_CAR_HEADLIGHTS> carDirection{};
    std::array<int, MAX_CAR_HEADLIGHTS> carColor{};
    std::array<int, MAX_CAR_HEADLIGHTS> carCutOff{};
    std::array<int, MAX_CAR_HEADLIGHTS> carOuterCutOff{};
    std::array<int, MAX_CAR_HEADLIGHTS> carConstant{};
    std::array<int, MAX_CAR_HEADLIGHTS> carLinear{};
    std::array<int, MAX_CAR_HEADLIGHTS> carQuadratic{};
};

LightingUniformLocations gLightingUniforms;
unsigned int gLightingUniformProgram = 0;

void cacheLightingUniformLocations(unsigned int shaderProgram) {
    if (gLightingUniformProgram == shaderProgram) {
        return;
    }

    gLightingUniformProgram = shaderProgram;
    gLightingUniforms.viewPos = glGetUniformLocation(shaderProgram, "viewPos");
    gLightingUniforms.globalAmbientStrength = glGetUniformLocation(shaderProgram, "globalAmbientStrength");
    gLightingUniforms.spotlightAmbientStrength = glGetUniformLocation(shaderProgram, "spotlightAmbientStrength");
    gLightingUniforms.headlightAmbientStrength = glGetUniformLocation(shaderProgram, "headlightAmbientStrength");
    gLightingUniforms.sunDirection = glGetUniformLocation(shaderProgram, "sunDirection");
    gLightingUniforms.sunColor = glGetUniformLocation(shaderProgram, "sunColor");
    gLightingUniforms.sunAmbientStrength = glGetUniformLocation(shaderProgram, "sunAmbientStrength");
    gLightingUniforms.sunDiffuseStrength = glGetUniformLocation(shaderProgram, "sunDiffuseStrength");
    gLightingUniforms.sunSpecularStrength = glGetUniformLocation(shaderProgram, "sunSpecularStrength");
    gLightingUniforms.numBuildingLights = glGetUniformLocation(shaderProgram, "numBuildingLights");
    gLightingUniforms.numCarHeadlights = glGetUniformLocation(shaderProgram, "numCarHeadlights");

    for (int i = 0; i < MAX_BUILDING_LIGHTS; ++i) {
        const std::string prefix = "buildingLights[" + std::to_string(i) + "].";
        gLightingUniforms.position[i] = glGetUniformLocation(shaderProgram, (prefix + "position").c_str());
        gLightingUniforms.direction[i] = glGetUniformLocation(shaderProgram, (prefix + "direction").c_str());
        gLightingUniforms.color[i] = glGetUniformLocation(shaderProgram, (prefix + "color").c_str());
        gLightingUniforms.cutOff[i] = glGetUniformLocation(shaderProgram, (prefix + "cutOff").c_str());
        gLightingUniforms.outerCutOff[i] = glGetUniformLocation(shaderProgram, (prefix + "outerCutOff").c_str());
        gLightingUniforms.constant[i] = glGetUniformLocation(shaderProgram, (prefix + "constant").c_str());
        gLightingUniforms.linear[i] = glGetUniformLocation(shaderProgram, (prefix + "linear").c_str());
        gLightingUniforms.quadratic[i] = glGetUniformLocation(shaderProgram, (prefix + "quadratic").c_str());
    }

    for (int i = 0; i < MAX_CAR_HEADLIGHTS; ++i) {
        const std::string prefix = "carHeadlights[" + std::to_string(i) + "].";
        gLightingUniforms.carPosition[i] = glGetUniformLocation(shaderProgram, (prefix + "position").c_str());
        gLightingUniforms.carDirection[i] = glGetUniformLocation(shaderProgram, (prefix + "direction").c_str());
        gLightingUniforms.carColor[i] = glGetUniformLocation(shaderProgram, (prefix + "color").c_str());
        gLightingUniforms.carCutOff[i] = glGetUniformLocation(shaderProgram, (prefix + "cutOff").c_str());
        gLightingUniforms.carOuterCutOff[i] = glGetUniformLocation(shaderProgram, (prefix + "outerCutOff").c_str());
        gLightingUniforms.carConstant[i] = glGetUniformLocation(shaderProgram, (prefix + "constant").c_str());
        gLightingUniforms.carLinear[i] = glGetUniformLocation(shaderProgram, (prefix + "linear").c_str());
        gLightingUniforms.carQuadratic[i] = glGetUniformLocation(shaderProgram, (prefix + "quadratic").c_str());
    }
}

std::array<CarHeadlightState, MAX_CAR_HEADLIGHTS> createCarHeadlights(const glm::mat4& carModel) {
    std::array<CarHeadlightState, MAX_CAR_HEADLIGHTS> headlights{};
    constexpr std::array<float, MAX_CAR_HEADLIGHTS> sideSigns = {1.0f, -1.0f};

    const glm::vec3 localDirection = glm::normalize(glm::vec3(1.0f, CAR_HEADLIGHT_DIRECTION_Y, 0.0f));
    const glm::vec3 worldDirection = glm::normalize(glm::vec3(carModel * glm::vec4(localDirection, 0.0f)));
    const glm::vec3 headlightColor(CAR_HEADLIGHT_COLOR_R,
                                   CAR_HEADLIGHT_COLOR_G,
                                   CAR_HEADLIGHT_COLOR_B);

    for (int i = 0; i < MAX_CAR_HEADLIGHTS; ++i) {
        const glm::vec3 localPos(CAR_HEADLIGHT_LOCAL_X,
                                 CAR_HEADLIGHT_LOCAL_Y,
                                 sideSigns[static_cast<std::size_t>(i)] * CAR_HEADLIGHT_LOCAL_Z_OFFSET);

        headlights[static_cast<std::size_t>(i)] = {
            glm::vec3(carModel * glm::vec4(localPos, 1.0f)),
            worldDirection,
            headlightColor,
        };
    }

    return headlights;
}

glm::vec3 windmillCenterForBuilding(const BuildingModels::Instance& building) {
    const float edgeOffset = std::max(0.0f, building.halfWidth - BUILDING_ROOF_ATTACHMENT_EDGE_MARGIN);
    return BuildingModels::roofCenter(building)
        + glm::vec3(BuildingModels::roadFacingSignX(building) * edgeOffset, 0.0f, 0.0f);
}

float windmillBeamTransmission(const BuildingModels::Instance& building,
                               const glm::vec3& lightPosition,
                               const glm::vec3& lightDirection,
                               float currentWindmillAngle) {
    const glm::vec3 windmillCenter = windmillCenterForBuilding(building);
    const glm::vec3 rotorAxis = glm::vec3(BuildingModels::roadFacingSignX(building), 0.0f, 0.0f);

    const float denom = glm::dot(lightDirection, rotorAxis);
    if (std::abs(denom) < WINDMILL_LIGHT_OCCLUSION_PARALLEL_EPSILON) {
        return 1.0f;
    }

    const float t = glm::dot(windmillCenter - lightPosition, rotorAxis) / denom;
    if (t <= 0.0f) {
        return 1.0f;
    }

    const glm::vec3 intersection = lightPosition + (lightDirection * t);
    const glm::vec3 relative = intersection - windmillCenter;
    const glm::vec3 relativeOnDisk = relative - (rotorAxis * glm::dot(relative, rotorAxis));

    const float radialDistance = glm::length(relativeOnDisk);
    const float diskMask = 1.0f - glm::smoothstep(WINDMILL_RADIUS - WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                                   WINDMILL_RADIUS + WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                                   radialDistance);

    if (diskMask <= 0.0f) {
        return 1.0f;
    }

    const float hubRadius = WINDMILL_BLADE_WIDTH * WINDMILL_LIGHT_HUB_RADIUS_FACTOR;
    const float hubOcclusion = diskMask
        * (1.0f - glm::smoothstep(hubRadius - WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                  hubRadius + WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                  radialDistance));

    glm::mat4 rotorRotation(1.0f);
    rotorRotation = glm::rotate(rotorRotation, BuildingModels::roadFacingYawRadians(building), glm::vec3(0.0f, 1.0f, 0.0f));
    rotorRotation = glm::rotate(rotorRotation, currentWindmillAngle, glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 bladeAxisA = glm::normalize(glm::vec3(rotorRotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    const glm::vec3 bladeAxisB = glm::normalize(glm::vec3(rotorRotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));

    auto bladeOcclusionForAxis = [&](const glm::vec3& axis) {
        const float along = std::abs(glm::dot(relativeOnDisk, axis));
        const glm::vec3 perpendicular = relativeOnDisk - (axis * glm::dot(relativeOnDisk, axis));
        const float perpDistance = glm::length(perpendicular);

        const float lengthMask = 1.0f - glm::smoothstep(WINDMILL_RADIUS - WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                WINDMILL_RADIUS + WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                                        along);
        const float widthMask = 1.0f - glm::smoothstep((WINDMILL_BLADE_WIDTH * 0.5f) - WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                   (WINDMILL_BLADE_WIDTH * 0.5f) + WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE,
                                                       perpDistance);

        return diskMask * lengthMask * widthMask;
    };

    const float bladeOcclusion = std::max(bladeOcclusionForAxis(bladeAxisA),
                                          bladeOcclusionForAxis(bladeAxisB));

    const float hubDarkness = hubOcclusion * (1.0f - WINDMILL_LIGHT_HUB_MIN_TRANSMISSION);
    const float bladeDarkness = bladeOcclusion * (1.0f - WINDMILL_LIGHT_BLADE_MIN_TRANSMISSION);
    const float totalDarkness = std::max(hubDarkness, bladeDarkness);

    return glm::clamp(1.0f - totalDarkness, 0.0f, 1.0f);
}

}  // namespace

DayNightCycleState evaluateDayNightCycle(float currentTime) {
    DayNightCycleState state;
    state.globalAmbientStrength = GLOBAL_AMBIENT_STRENGTH;
    state.spotlightAmbientStrength = SPOTLIGHT_AMBIENT_STRENGTH;
    state.headlightAmbientStrength = CAR_HEADLIGHT_AMBIENT_STRENGTH;

    state.buildingLightColorScale = DAY_NIGHT_BUILDING_LIGHT_COLOR_SCALE_DAY;
    state.headlightColorScale = DAY_NIGHT_HEADLIGHT_COLOR_SCALE_DAY;

    state.sunAmbientStrength = SUN_AMBIENT_STRENGTH_DAY;
    state.sunDiffuseStrength = SUN_DIFFUSE_STRENGTH_DAY;
    state.sunSpecularStrength = SUN_SPECULAR_STRENGTH_DAY;

    state.skyColor = glm::vec3(SKY_DAY_R,
                               SKY_DAY_G,
                               SKY_DAY_B);
    state.sunDirection = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
    state.sunColor = glm::vec3(SUN_COLOR_DAY_R,
                               SUN_COLOR_DAY_G,
                               SUN_COLOR_DAY_B);

    if (!DAY_NIGHT_CYCLE_ENABLED) {
        return state;
    }

    const float cycleDuration = std::max(DAY_NIGHT_CYCLE_DURATION_SECONDS, 1.0f);
    float cycleProgress = std::fmod(currentTime / cycleDuration, 1.0f);
    if (cycleProgress < 0.0f) {
        cycleProgress += 1.0f;
    }
    state.cycleProgress = cycleProgress;

    const float orbitAngle = (cycleProgress * glm::two_pi<float>()) + glm::radians(DAY_NIGHT_SUN_PHASE_OFFSET_DEG);
    const glm::vec3 orbitPosition(std::cos(orbitAngle), std::sin(orbitAngle), DAY_NIGHT_SUN_ORBIT_Z_BIAS);
    state.sunDirection = glm::normalize(-orbitPosition);

    const float sunHeight = glm::clamp(orbitPosition.y, -1.0f, 1.0f);
    state.daylightFactor = glm::smoothstep(DAY_NIGHT_DAYLIGHT_START_HEIGHT,
                                           DAY_NIGHT_DAYLIGHT_END_HEIGHT,
                                           sunHeight);
    state.nightFactor = 1.0f - state.daylightFactor;

    const float horizonBandFactor = 1.0f - glm::smoothstep(DAY_NIGHT_HORIZON_BAND_MIN_ABS_HEIGHT,
                                                           DAY_NIGHT_HORIZON_BAND_MAX_ABS_HEIGHT,
                                                           std::abs(sunHeight));
    const float horizonBlend = horizonBandFactor * DAY_NIGHT_HORIZON_TINT_STRENGTH;

    const glm::vec3 daySky(SKY_DAY_R, SKY_DAY_G, SKY_DAY_B);
    const glm::vec3 nightSky(SKY_NIGHT_R, SKY_NIGHT_G, SKY_NIGHT_B);
    const glm::vec3 horizonSky(SKY_HORIZON_R, SKY_HORIZON_G, SKY_HORIZON_B);

    state.skyColor = glm::mix(nightSky, daySky, state.daylightFactor);
    state.skyColor = glm::mix(state.skyColor, horizonSky, horizonBlend);

    const glm::vec3 daySunColor(SUN_COLOR_DAY_R,
                                SUN_COLOR_DAY_G,
                                SUN_COLOR_DAY_B);
    const glm::vec3 nightSunColor(SUN_COLOR_NIGHT_R,
                                  SUN_COLOR_NIGHT_G,
                                  SUN_COLOR_NIGHT_B);
    const glm::vec3 horizonSunColor(SUN_COLOR_HORIZON_R,
                                    SUN_COLOR_HORIZON_G,
                                    SUN_COLOR_HORIZON_B);

    state.sunColor = glm::mix(nightSunColor, daySunColor, state.daylightFactor);
    state.sunColor = glm::mix(state.sunColor, horizonSunColor, horizonBlend);

    state.globalAmbientStrength = glm::mix(GLOBAL_AMBIENT_NIGHT,
                                           GLOBAL_AMBIENT_DAY,
                                           state.daylightFactor);
    state.spotlightAmbientStrength = glm::mix(SPOTLIGHT_AMBIENT_NIGHT,
                                              SPOTLIGHT_AMBIENT_DAY,
                                              state.daylightFactor);
    state.headlightAmbientStrength = glm::mix(HEADLIGHT_AMBIENT_NIGHT,
                                              HEADLIGHT_AMBIENT_DAY,
                                              state.daylightFactor);

    state.buildingLightColorScale = glm::mix(DAY_NIGHT_BUILDING_LIGHT_COLOR_SCALE_NIGHT,
                                             DAY_NIGHT_BUILDING_LIGHT_COLOR_SCALE_DAY,
                                             state.daylightFactor);
    state.headlightColorScale = glm::mix(DAY_NIGHT_HEADLIGHT_COLOR_SCALE_NIGHT,
                                         DAY_NIGHT_HEADLIGHT_COLOR_SCALE_DAY,
                                         state.daylightFactor);

    state.sunAmbientStrength = glm::mix(SUN_AMBIENT_STRENGTH_NIGHT,
                                        SUN_AMBIENT_STRENGTH_DAY,
                                        state.daylightFactor);
    state.sunDiffuseStrength = glm::mix(SUN_DIFFUSE_STRENGTH_NIGHT,
                                        SUN_DIFFUSE_STRENGTH_DAY,
                                        state.daylightFactor);
    state.sunSpecularStrength = glm::mix(SUN_SPECULAR_STRENGTH_NIGHT,
                                         SUN_SPECULAR_STRENGTH_DAY,
                                         state.daylightFactor);

    return state;
}

std::vector<BuildingLight> createBuildingLights(const BuildingModels::Scene& scene) {
    std::vector<BuildingLight> lights;

    std::vector<BuildingModels::LightRigConfig> rigConfigs = BuildingModels::createLightRigConfigs(scene);
    lights.reserve(rigConfigs.size());

    for (const BuildingModels::LightRigConfig& rig : rigConfigs) {
        BuildingLight light;
        light.basePosition = rig.basePosition;
        light.baseDirection = rig.baseDirection;
        light.color = rig.color;
        light.currentSwingAngle = 0.0f;
        light.swingSpeed = rig.swingSpeed;
        lights.push_back(light);
    }

    return lights;
}

float swingAngleForLight(const BuildingLight& light, float time) {
    const float maxSwing = glm::radians(BUILDING_LIGHT_SWING_MAX_ANGLE_DEG);
    return std::sin(time * light.swingSpeed) * maxSwing;
}

glm::vec3 directionForLight(const BuildingLight& light, float time) {
    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), swingAngleForLight(light, time), glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::normalize(glm::vec3(rotation * glm::vec4(light.baseDirection, 0.0f)));
}

glm::vec3 positionForLight(const BuildingLight& light, float time) {
    const glm::vec3 horizontalForward = glm::normalize(glm::vec3(light.baseDirection.x, 0.0f, light.baseDirection.z));
    const glm::vec3 neutralNozzleOffset = horizontalForward * (BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_NOZZLE_X_FACTOR);

    const glm::vec3 pivotPosition = light.basePosition
        - glm::vec3(0.0f, BUILDING_LIGHT_LAMP_CENTER_Y, 0.0f)
        - neutralNozzleOffset;

    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), swingAngleForLight(light, time), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 swungNozzleOffset = glm::vec3(rotation * glm::vec4(neutralNozzleOffset, 0.0f));

    return pivotPosition
        + glm::vec3(0.0f, BUILDING_LIGHT_LAMP_CENTER_Y, 0.0f)
        + swungNozzleOffset;
}

void setupLighting(unsigned int shaderProgram,
                   std::vector<BuildingLight>& lights,
                   const BuildingModels::Scene& towerScene,
                   float currentWindmillAngle,
                   const glm::vec3& viewPos,
                   const glm::mat4& carModel,
                   bool headlightsOn,
                   float time,
                   const DayNightCycleState& dayNightState,
                   bool applyWindmillOcclusion) {
    cacheLightingUniformLocations(shaderProgram);

    glUseProgram(shaderProgram);
    glUniform3fv(gLightingUniforms.viewPos, 1, glm::value_ptr(viewPos));
    glUniform1f(gLightingUniforms.globalAmbientStrength, dayNightState.globalAmbientStrength);
    glUniform1f(gLightingUniforms.spotlightAmbientStrength, dayNightState.spotlightAmbientStrength);
    glUniform1f(gLightingUniforms.headlightAmbientStrength, dayNightState.headlightAmbientStrength);
    glUniform3fv(gLightingUniforms.sunDirection, 1, glm::value_ptr(dayNightState.sunDirection));
    glUniform3fv(gLightingUniforms.sunColor, 1, glm::value_ptr(dayNightState.sunColor));
    glUniform1f(gLightingUniforms.sunAmbientStrength, dayNightState.sunAmbientStrength);
    glUniform1f(gLightingUniforms.sunDiffuseStrength, dayNightState.sunDiffuseStrength);
    glUniform1f(gLightingUniforms.sunSpecularStrength, dayNightState.sunSpecularStrength);

    static const float buildingInnerCutoff = glm::cos(glm::radians(BUILDING_LIGHT_INNER_CUTOFF_DEG));
    static const float buildingOuterCutoff = glm::cos(glm::radians(BUILDING_LIGHT_OUTER_CUTOFF_DEG));
    static const float carInnerCutoff = glm::cos(glm::radians(CAR_HEADLIGHT_INNER_CUTOFF_DEG));
    static const float carOuterCutoff = glm::cos(glm::radians(CAR_HEADLIGHT_OUTER_CUTOFF_DEG));

    const int activeLightCount = std::min(static_cast<int>(lights.size()), MAX_BUILDING_LIGHTS);
    glUniform1i(gLightingUniforms.numBuildingLights, activeLightCount);

    for (int i = 0; i < activeLightCount; i++) {
        lights[i].currentSwingAngle = swingAngleForLight(lights[i], time);
        glm::vec3 currentPos = positionForLight(lights[i], time);
        glm::vec3 currentDir = directionForLight(lights[i], time);
        glm::vec3 effectiveColor = lights[i].color;

        if (applyWindmillOcclusion && i < static_cast<int>(towerScene.instances.size())) {
            const float transmission = windmillBeamTransmission(towerScene.instances[static_cast<std::size_t>(i)],
                                                                currentPos,
                                                                currentDir,
                                                                currentWindmillAngle);
            effectiveColor *= transmission;
        }

        effectiveColor *= dayNightState.buildingLightColorScale;

        glUniform3fv(gLightingUniforms.position[i], 1, glm::value_ptr(currentPos));
        glUniform3fv(gLightingUniforms.direction[i], 1, glm::value_ptr(currentDir));
        glUniform3fv(gLightingUniforms.color[i], 1, glm::value_ptr(effectiveColor));

        glUniform1f(gLightingUniforms.cutOff[i], buildingInnerCutoff);
        glUniform1f(gLightingUniforms.outerCutOff[i], buildingOuterCutoff);

        glUniform1f(gLightingUniforms.constant[i], BUILDING_LIGHT_ATTENUATION_CONSTANT);
        glUniform1f(gLightingUniforms.linear[i], BUILDING_LIGHT_ATTENUATION_LINEAR);
        glUniform1f(gLightingUniforms.quadratic[i], BUILDING_LIGHT_ATTENUATION_QUADRATIC);
    }

    const int activeCarHeadlightCount = headlightsOn ? MAX_CAR_HEADLIGHTS : 0;
    glUniform1i(gLightingUniforms.numCarHeadlights, activeCarHeadlightCount);

    if (activeCarHeadlightCount > 0) {
        const std::array<CarHeadlightState, MAX_CAR_HEADLIGHTS> carHeadlights = createCarHeadlights(carModel);

        for (int i = 0; i < activeCarHeadlightCount; ++i) {
            const CarHeadlightState& headlight = carHeadlights[static_cast<std::size_t>(i)];
            const glm::vec3 effectiveColor = headlight.color * dayNightState.headlightColorScale;

            glUniform3fv(gLightingUniforms.carPosition[i], 1, glm::value_ptr(headlight.position));
            glUniform3fv(gLightingUniforms.carDirection[i], 1, glm::value_ptr(headlight.direction));
            glUniform3fv(gLightingUniforms.carColor[i], 1, glm::value_ptr(effectiveColor));
            glUniform1f(gLightingUniforms.carCutOff[i], carInnerCutoff);
            glUniform1f(gLightingUniforms.carOuterCutOff[i], carOuterCutoff);
            glUniform1f(gLightingUniforms.carConstant[i], CAR_HEADLIGHT_ATTENUATION_CONSTANT);
            glUniform1f(gLightingUniforms.carLinear[i], CAR_HEADLIGHT_ATTENUATION_LINEAR);
            glUniform1f(gLightingUniforms.carQuadratic[i], CAR_HEADLIGHT_ATTENUATION_QUADRATIC);
        }
    }
}

}  // namespace Lighting
