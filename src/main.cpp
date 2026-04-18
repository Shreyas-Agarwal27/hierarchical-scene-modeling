#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <vector>
#include <algorithm>

#include "constants.h"
#include "building_models.h"
#include "collision.h"
#include "mesh.h"
#include "object_meshes.h"
#include "object_renderer.h"
#include "shader_utils.h"
#include "texture_utils.h"
#include "camera.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float frameTimer = 0.0f;
int frameCount = 0;

float carSpeed = CAR_DEFAULT_SPEED;
float carX = CAR_START_X;
float carZ = CAR_START_Z;
float carAngle = glm::radians(CAR_START_ANGLE_DEG);
bool carCrashed = false;
bool showHitboxes = false;
bool resetWorldRequested = false;
bool headlightsEnabled = true;

float windmillAngle = 0.0f;
float windmillSpeed = WINDMILL_DEFAULT_SPEED;

Camera camera;

namespace {

struct BuildingLight {
    glm::vec3 basePosition;
    glm::vec3 baseDirection; // Pointing at the road
    glm::vec3 color;
    
    float currentSwingAngle = 0.0f;
    float swingSpeed = BUILDING_LIGHT_SWING_BASE_SPEED;
};

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

struct StaticObstacle {
    Collision::AABB2D bounds;
    float height;
};

Collision::AABB2D createAABBFromCenter(float centerX, float centerZ, float sizeX, float sizeZ) {
    const float halfX = sizeX * 0.5f;
    const float halfZ = sizeZ * 0.5f;

    return {
        centerX - halfX,
        centerX + halfX,
        centerZ - halfZ,
        centerZ + halfZ,
    };
}

std::vector<StaticObstacle> createStaticObstacles(const std::vector<BuildingModels::Instance>& buildings) {
    std::vector<StaticObstacle> obstacles;
    obstacles.reserve(buildings.size() + 4);

    for (const BuildingModels::Instance& building : buildings) {
        obstacles.push_back({
            createAABBFromCenter(building.position.x,
                                 building.position.z,
                                 building.halfWidth * 2.0f,
                                 building.halfDepth * 2.0f),
            building.roofY,
        });
    }

    const float halfSize = WORLD_SIZE * 0.5f;
    const float verticalWallDepth = WORLD_SIZE + WALL_THICKNESS;

    obstacles.push_back({createAABBFromCenter(0.0f, halfSize, WORLD_SIZE, WALL_THICKNESS), WALL_HEIGHT});
    obstacles.push_back({createAABBFromCenter(0.0f, -halfSize, WORLD_SIZE, WALL_THICKNESS), WALL_HEIGHT});
    obstacles.push_back({createAABBFromCenter(halfSize, 0.0f, WALL_THICKNESS, verticalWallDepth), WALL_HEIGHT});
    obstacles.push_back({createAABBFromCenter(-halfSize, 0.0f, WALL_THICKNESS, verticalWallDepth), WALL_HEIGHT});

    return obstacles;
}

std::vector<Collision::AABB2D> extractAABBs(const std::vector<StaticObstacle>& obstacles) {
    std::vector<Collision::AABB2D> aabbs;
    aabbs.reserve(obstacles.size());

    for (const StaticObstacle& obstacle : obstacles) {
        aabbs.push_back(obstacle.bounds);
    }

    return aabbs;
}

Collision::OBB2D carHitboxAt(float x, float z, float angleRadians) {
    return {
        glm::vec2(x, z),
        CAR_HITBOX_LENGTH * 0.5f,
        CAR_HITBOX_WIDTH * 0.5f,
        angleRadians,
    };
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

    // Reconstruct the gimbal pivot point from the neutral nozzle position.
    const glm::vec3 pivotPosition = light.basePosition
        - glm::vec3(0.0f, BUILDING_LIGHT_LAMP_CENTER_Y, 0.0f)
        - neutralNozzleOffset;

    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), swingAngleForLight(light, time), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 swungNozzleOffset = glm::vec3(rotation * glm::vec4(neutralNozzleOffset, 0.0f));

    return pivotPosition
        + glm::vec3(0.0f, BUILDING_LIGHT_LAMP_CENTER_Y, 0.0f)
        + swungNozzleOffset;
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

float gimbalFacingYawRadians(const BuildingModels::Instance& building) {
    // Gimbal mesh lamp body points toward +X in local space.
    // Use configured yaw so lamp forward aligns with road-facing +/-X.
    return (BuildingModels::roadFacingSignX(building) > 0.0f)
        ? glm::radians(BUILDING_LIGHT_ROAD_FACING_YAW_NEGATIVE_X_DEG)
        : glm::radians(BUILDING_LIGHT_ROAD_FACING_YAW_POSITIVE_X_DEG);
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

void drawBuildings(unsigned int shaderProgram,
                            const glm::mat4& projection,
                            const glm::mat4& view,
                            BuildingModels::Scene& towerScene,
                            const std::vector<BuildingLight>& buildingLights,
                            Mesh& windmillMesh,
                            Mesh& lightGimbalBaseMesh,
                            Mesh& lightGimbalHeadMesh,
                            float currentWindmillAngle,
                            float currentTime) {
    for (std::size_t buildingIndex = 0; buildingIndex < towerScene.instances.size(); ++buildingIndex) {
        const BuildingModels::Instance& building = towerScene.instances[buildingIndex];

        // draw building
        const glm::mat4 buildingTransform = BuildingModels::modelTransformFor(towerScene, building);

        ObjectRenderer::drawBuilding(shaderProgram,
                                     projection,
                                     view,
                                     BuildingModels::meshFor(towerScene, building),
                                     buildingTransform,
                                     glm::vec3(BUILDING_COLOR_R, BUILDING_COLOR_G, BUILDING_COLOR_B));

        if (buildingIndex < buildingLights.size()) {
            const BuildingLight& light = buildingLights[buildingIndex];

            glm::mat4 gimbalBaseTransform = glm::mat4(1.0f);
            gimbalBaseTransform = glm::translate(gimbalBaseTransform,
                                                 BuildingModels::roofCenter(building) + glm::vec3(0.0f, BUILDING_LIGHT_HEIGHT_OFFSET, 0.0f));

            const float roadFacingYaw = gimbalFacingYawRadians(building);
            gimbalBaseTransform = glm::rotate(gimbalBaseTransform, roadFacingYaw, glm::vec3(0.0f, 1.0f, 0.0f));

            const glm::vec3 gimbalColor = light.color;

            ObjectRenderer::drawBuilding(shaderProgram,
                                         projection,
                                         view,
                                         lightGimbalBaseMesh,
                                         gimbalBaseTransform,
                                         gimbalColor);

            glm::mat4 gimbalHeadTransform = gimbalBaseTransform;
            gimbalHeadTransform = glm::translate(gimbalHeadTransform,
                                                 glm::vec3(0.0f, BUILDING_LIGHT_GIMBAL_PIVOT_HEIGHT, 0.0f));
            gimbalHeadTransform = glm::rotate(gimbalHeadTransform,
                                              swingAngleForLight(light, currentTime),
                                              glm::vec3(0.0f, 1.0f, 0.0f));

            ObjectRenderer::drawBuilding(shaderProgram,
                                         projection,
                                         view,
                                         lightGimbalHeadMesh,
                                         gimbalHeadTransform,
                                         gimbalColor);
        }

        glm::mat4 windmillTransform = glm::mat4(1.0f);
        windmillTransform = glm::translate(windmillTransform,
                                           windmillCenterForBuilding(building));
        windmillTransform = glm::rotate(windmillTransform, BuildingModels::roadFacingYawRadians(building), glm::vec3(0.0f, 1.0f, 0.0f));

        windmillTransform = glm::rotate(windmillTransform, currentWindmillAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        // can draw any colored mesh
        ObjectRenderer::drawBuilding(shaderProgram, projection, view, 
                                     windmillMesh, 
                                     windmillTransform, 
                                     glm::vec3(WINDMILL_COLOR_R, WINDMILL_COLOR_G, WINDMILL_COLOR_B));
    }
}

void drawCarHeadlightsVisual(unsigned int shaderProgram,
                             const glm::mat4& projection,
                             const glm::mat4& view,
                             Mesh& unitBoxMesh,
                             const glm::mat4& carModel,
                             bool headlightsOn) {
    constexpr std::array<float, MAX_CAR_HEADLIGHTS> sideSigns = {1.0f, -1.0f};
    const glm::vec3 lightColor = headlightsOn
        ? glm::vec3(CAR_HEADLIGHT_COLOR_R, CAR_HEADLIGHT_COLOR_G, CAR_HEADLIGHT_COLOR_B)
        : glm::vec3(0.45f, 0.45f, 0.45f);

    for (float sign : sideSigns) {
        glm::mat4 headlightModel = carModel;
        headlightModel = glm::translate(headlightModel,
                                        glm::vec3(CAR_HEADLIGHT_LOCAL_X,
                                                  CAR_HEADLIGHT_LOCAL_Y,
                                                  sign * CAR_HEADLIGHT_LOCAL_Z_OFFSET));
        headlightModel = glm::scale(headlightModel,
                                    glm::vec3(CAR_HEADLIGHT_VISUAL_DEPTH,
                                              CAR_HEADLIGHT_VISUAL_HEIGHT,
                                              CAR_HEADLIGHT_VISUAL_WIDTH));

        ObjectRenderer::drawBuilding(shaderProgram,
                                     projection,
                                     view,
                                     unitBoxMesh,
                                     headlightModel,
                                     lightColor);
    }
}

void setupLighting(unsigned int shaderProgram,
                   std::vector<BuildingLight>& lights,
                   const BuildingModels::Scene& towerScene,
                   float currentWindmillAngle,
                   const glm::vec3& viewPos,
                   const glm::mat4& carModel,
                   bool headlightsOn,
                   float time) {
    cacheLightingUniformLocations(shaderProgram);

    glUseProgram(shaderProgram);
    glUniform3fv(gLightingUniforms.viewPos, 1, glm::value_ptr(viewPos));
    glUniform1f(gLightingUniforms.globalAmbientStrength, GLOBAL_AMBIENT_STRENGTH);
    glUniform1f(gLightingUniforms.spotlightAmbientStrength, SPOTLIGHT_AMBIENT_STRENGTH);
    glUniform1f(gLightingUniforms.headlightAmbientStrength, CAR_HEADLIGHT_AMBIENT_STRENGTH);

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

        if (i < static_cast<int>(towerScene.instances.size())) {
            const float transmission = windmillBeamTransmission(towerScene.instances[static_cast<std::size_t>(i)],
                                                                currentPos,
                                                                currentDir,
                                                                currentWindmillAngle);
            effectiveColor *= transmission;
        }

        glUniform3fv(gLightingUniforms.position[i], 1, glm::value_ptr(currentPos));
        glUniform3fv(gLightingUniforms.direction[i], 1, glm::value_ptr(currentDir));
        glUniform3fv(gLightingUniforms.color[i], 1, glm::value_ptr(effectiveColor));
        
        // Spotlight cone geometry
        glUniform1f(gLightingUniforms.cutOff[i], buildingInnerCutoff);
        glUniform1f(gLightingUniforms.outerCutOff[i], buildingOuterCutoff);
        
        // Attenuation configuration for realistic falloff
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

            glUniform3fv(gLightingUniforms.carPosition[i], 1, glm::value_ptr(headlight.position));
            glUniform3fv(gLightingUniforms.carDirection[i], 1, glm::value_ptr(headlight.direction));
            glUniform3fv(gLightingUniforms.carColor[i], 1, glm::value_ptr(headlight.color));
            glUniform1f(gLightingUniforms.carCutOff[i], carInnerCutoff);
            glUniform1f(gLightingUniforms.carOuterCutOff[i], carOuterCutoff);
            glUniform1f(gLightingUniforms.carConstant[i], CAR_HEADLIGHT_ATTENUATION_CONSTANT);
            glUniform1f(gLightingUniforms.carLinear[i], CAR_HEADLIGHT_ATTENUATION_LINEAR);
            glUniform1f(gLightingUniforms.carQuadratic[i], CAR_HEADLIGHT_ATTENUATION_QUADRATIC);
        }
    }
}

void drawHitboxes(unsigned int shaderProgram,
                 const glm::mat4& projection,
                 const glm::mat4& view,
                 Mesh& unitBoxMesh,
                 const std::vector<StaticObstacle>& obstacles,
                 float currentCarX,
                 float currentCarZ,
                 float currentCarAngle) {
    glLineWidth(HITBOX_LINE_WIDTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (const StaticObstacle& obstacle : obstacles) {
        const float width = (obstacle.bounds.maxX - obstacle.bounds.minX) + (2.0f * HITBOX_VISUAL_PADDING);
        const float depth = (obstacle.bounds.maxZ - obstacle.bounds.minZ) + (2.0f * HITBOX_VISUAL_PADDING);
        const float height = obstacle.height + HITBOX_VISUAL_PADDING;
        const float centerX = 0.5f * (obstacle.bounds.minX + obstacle.bounds.maxX);
        const float centerZ = 0.5f * (obstacle.bounds.minZ + obstacle.bounds.maxZ);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, height * 0.5f, centerZ));
        model = glm::scale(model, glm::vec3(width, height, depth));

        ObjectRenderer::drawBuilding(shaderProgram,
                                     projection,
                                     view,
                                     unitBoxMesh,
                                     model,
                                     glm::vec3(1.0f, 0.15f, 0.15f));
    }

    glm::mat4 carHitboxModel = glm::mat4(1.0f);
    carHitboxModel = glm::translate(carHitboxModel, glm::vec3(currentCarX, CAR_HITBOX_HEIGHT * 0.5f, currentCarZ));
    carHitboxModel = glm::rotate(carHitboxModel, currentCarAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    carHitboxModel = glm::scale(carHitboxModel,
                                glm::vec3(CAR_HITBOX_LENGTH + (2.0f * HITBOX_VISUAL_PADDING),
                                          CAR_HITBOX_HEIGHT + HITBOX_VISUAL_PADDING,
                                          CAR_HITBOX_WIDTH + (2.0f * HITBOX_VISUAL_PADDING)));

    ObjectRenderer::drawBuilding(shaderProgram,
                                 projection,
                                 view,
                                 unitBoxMesh,
                                 carHitboxModel,
                                 glm::vec3(0.2f, 1.0f, 0.2f));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}

}  // namespace

void resetWorldState() {
    carSpeed = CAR_DEFAULT_SPEED;
    carX = CAR_START_X;
    carZ = CAR_START_Z;
    carAngle = glm::radians(CAR_START_ANGLE_DEG);
    carCrashed = false;

    windmillAngle = 0.0f;
    windmillSpeed = WINDMILL_DEFAULT_SPEED;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) camera.setMode(CameraMode::SKY_VIEW);
        if (key == GLFW_KEY_2) camera.setMode(CameraMode::CAR_VIEW);
        if (key == GLFW_KEY_3) camera.setMode(CameraMode::GROUND_VIEW);
        if (key == GLFW_KEY_4) camera.setMode(CameraMode::LIGHTSOURCE_VIEW);
        if (key == GLFW_KEY_5) camera.setMode(CameraMode::HELICOPTER_CAM);
        if (key == GLFW_KEY_B) showHitboxes = !showHitboxes;
        if (key == GLFW_KEY_H) headlightsEnabled = !headlightsEnabled;
        if (key == GLFW_KEY_SPACE) resetWorldRequested = true;
    }

    if (camera.currentMode == CameraMode::GROUND_VIEW) {
        
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && camera.groundViewYaw < glm::radians(GROUND_CAMERA_MAX_ANGLE)) {
            camera.groundViewYaw += PAN_SPEED * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && camera.groundViewYaw > -glm::radians(GROUND_CAMERA_MAX_ANGLE)) {
            camera.groundViewYaw -= PAN_SPEED * deltaTime;
        }
    }
}

void processInput(GLFWwindow *window, float deltaTime) {
    if (!carCrashed) {
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            carSpeed += (CAR_SPEED_INC * 50.0f) * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            carSpeed -= (CAR_SPEED_INC * 50.0f) * deltaTime;
        }

        if (carSpeed != 0.0f) {
            // Multiply by 50 to scale the turn increment nicely with deltaTime
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
                carAngle += glm::radians(CAR_TURN_INC * 50.0f) * deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                carAngle -= glm::radians(CAR_TURN_INC * 50.0f) * deltaTime;
            }
        }
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            windmillSpeed -= WINDMILL_SPEED_INC * deltaTime;
        }
        else {
            windmillSpeed += WINDMILL_SPEED_INC * deltaTime;
        }
    }
}

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8); // request a stencil buffer from GLFW

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 3", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(window, key_callback);

    unsigned int shaderProgram = compileShaders("shaders/vertex.glsl", "shaders/fragment.glsl");

    unsigned int groundTexture = loadTexture("textures/grass.png");
    unsigned int trackTexture = loadTexture("textures/road.png");
    unsigned int brickTexture = loadTexture("textures/brick.jpg");
    unsigned int woodTexture = loadTexture("textures/wood.jpg");
    unsigned int towerTexture = loadTexture("textures/colormap.png");
    
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // matrix setup
    float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
    glm::mat4 projection = glm::perspective(glm::radians(CAMERA_FOV), aspectRatio, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

    // object setup
    Mesh ground = ObjectMeshes::createGround(groundTexture);
    Mesh track = ObjectMeshes::createTrack(trackTexture);

    Mesh carFrame = ObjectMeshes::createCarFrame();
    Mesh carWindows = ObjectMeshes::createCarWindows();
    Mesh carWheels = ObjectMeshes::createCarWheels();

    BuildingModels::Scene towerScene;
    try {
        towerScene = BuildingModels::loadTowerScene(towerTexture);
    }
    catch (const std::exception& ex) {
        std::cout << "Failed to load tower scene: " << ex.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    std::vector<BuildingLight> buildingLights = createBuildingLights(towerScene);
    std::vector<StaticObstacle> staticObstacles = createStaticObstacles(towerScene.instances);
    std::vector<Collision::AABB2D> staticObstacleAABBs = extractAABBs(staticObstacles);

    Mesh windmillMesh = ObjectMeshes::createWindmill(woodTexture);
    Mesh lightGimbalBaseMesh = ObjectMeshes::createLightGimbalBase();
    Mesh lightGimbalHeadMesh = ObjectMeshes::createLightGimbalHead();
    Mesh hitboxUnitBox = ObjectMeshes::createUnitBox();

    Mesh walls = ObjectMeshes::createWalls(brickTexture);

    ObjectRenderer::CarAppearance carAppearance = ObjectRenderer::defaultCarAppearance();

    glm::vec3 groundCamPos(0.0f);
    if (!towerScene.instances.empty()) {
        const BuildingModels::Instance& b = towerScene.instances[0];
        groundCamPos = b.position
            + glm::vec3(BuildingModels::roadFacingSignX(b) * (b.halfWidth + CAMERA_GROUND_FRONT_OFFSET),
                        CAMERA_GROUND_HEIGHT,
                        0.0f);
    }

    // game loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frameTimer += deltaTime;
        frameCount++;

        if (frameTimer >= 1.0f) {
            int fps = static_cast<int>(frameCount / frameTimer);
            std::string newTitle = "Assignment 3 - FPS: " + std::to_string(fps);
            glfwSetWindowTitle(window, newTitle.c_str());
            
            frameCount = 0;
            frameTimer = 0.0f;
        }

        // render
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (resetWorldRequested) {
            resetWorldState();
            resetWorldRequested = false;
        }

        glm::vec3 carPos(carX, 0.0f, carZ);
        glm::vec3 activeLightDir(DEFAULT_LIGHTSOURCE_VIEW_DIR_X,
                     DEFAULT_LIGHTSOURCE_VIEW_DIR_Y,
                     DEFAULT_LIGHTSOURCE_VIEW_DIR_Z);
        glm::vec3 activeLightPos(0.0f, DEFAULT_LIGHTSOURCE_VIEW_HEIGHT, 0.0f);
        if (!buildingLights.empty()) {
            activeLightDir = directionForLight(buildingLights[0], currentFrame);
            activeLightPos = positionForLight(buildingLights[0], currentFrame);
        }

        glm::mat4 view = camera.getViewMatrix(carPos, carAngle, groundCamPos, activeLightPos, activeLightDir);
        glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);

        processInput(window, deltaTime);
        windmillAngle += windmillSpeed * deltaTime;
        windmillAngle = std::fmod(windmillAngle, glm::two_pi<float>());

        if (!carCrashed) {
            const float nextCarX = carX + (carSpeed * std::cos(carAngle) * deltaTime);
            const float nextCarZ = carZ + (carSpeed * -std::sin(carAngle) * deltaTime);

            const Collision::OBB2D nextCarOBB = carHitboxAt(nextCarX, nextCarZ, carAngle);
            if (Collision::intersectsAny(nextCarOBB, staticObstacleAABBs, COLLISION_SAT_EPSILON)) {
                carCrashed = true;
                carSpeed = 0.0f;
            }
            else {
                carX = nextCarX;
                carZ = nextCarZ;
            }
        }

        glm::mat4 carModel = glm::mat4(1.0f);
        carModel = glm::translate(carModel, glm::vec3(carX, 0.0f, carZ));
        carModel = glm::rotate(carModel, carAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        carModel = glm::scale(carModel, glm::vec3(CAR_SCALE, CAR_SCALE, CAR_SCALE));

        setupLighting(shaderProgram,
                  buildingLights,
                  towerScene,
                  windmillAngle,
                  cameraPos,
                  carModel,
                  headlightsEnabled,
                  currentFrame);

        ObjectRenderer::drawFloor(shaderProgram, projection, view, track, ground);

        glm::mat4 wallTransform = glm::mat4(1.0f);
        ObjectRenderer::drawBuilding(shaderProgram, projection, view, walls, wallTransform, glm::vec3(1.0f, 1.0f, 1.0f));

        ObjectRenderer::drawCar(shaderProgram,
                    projection,
                    view,
                    carFrame,
                    carWindows,
                    carWheels,
                    carModel,
                    carAppearance);

        drawCarHeadlightsVisual(shaderProgram,
                    projection,
                    view,
                    hitboxUnitBox,
                    carModel,
                    headlightsEnabled);

        drawBuildings(shaderProgram,
                       projection,
                       view,
                       towerScene,
                       buildingLights,
                       windmillMesh,
                       lightGimbalBaseMesh,
                       lightGimbalHeadMesh,
                       windmillAngle,
                       currentFrame);

        if (showHitboxes) {
            drawHitboxes(shaderProgram,
                         projection,
                         view,
                         hitboxUnitBox,
                         staticObstacles,
                         carX,
                         carZ,
                         carAngle);
        }
        

        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    ground.cleanup();
    track.cleanup();
    carFrame.cleanup();
    carWindows.cleanup();
    carWheels.cleanup();
    windmillMesh.cleanup();
    lightGimbalBaseMesh.cleanup();
    lightGimbalHeadMesh.cleanup();
    hitboxUnitBox.cleanup();
    walls.cleanup();
    BuildingModels::cleanup(towerScene);
    glfwTerminate();
    return 0;
}