#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
#include "lighting.h"

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

using BuildingLight = Lighting::BuildingLight;
using DayNightCycleState = Lighting::DayNightCycleState;

namespace {

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
                                              Lighting::swingAngleForLight(light, currentTime),
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

    std::vector<BuildingLight> buildingLights = Lighting::createBuildingLights(towerScene);
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

        const DayNightCycleState dayNightState = Lighting::evaluateDayNightCycle(currentFrame);

        // render
        glClearColor(dayNightState.skyColor.r,
                     dayNightState.skyColor.g,
                     dayNightState.skyColor.b,
                     1.0f);
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
            activeLightDir = Lighting::directionForLight(buildingLights[0], currentFrame);
            activeLightPos = Lighting::positionForLight(buildingLights[0], currentFrame);
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

        Lighting::setupLighting(shaderProgram,
                    buildingLights,
                    towerScene,
                    windmillAngle,
                    cameraPos,
                    carModel,
                    headlightsEnabled,
                    currentFrame,
                    dayNightState);

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