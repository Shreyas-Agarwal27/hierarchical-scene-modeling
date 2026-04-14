#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

#include "constants.h"
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

float carSpeed = 5.0f;
float carX = CAR_START_X;
float carZ = CAR_START_Z;
float carAngle = 0.0f;

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

struct BuildingInstance {
    glm::vec3 position;
    int stories;
    std::size_t textureIndex;
};

int storyCountForBuildingIndex(int buildingIndex) {
    const int storyVariantCount = BUILDING_MAX_STORIES - BUILDING_MIN_STORIES + 1;
    return BUILDING_MIN_STORIES + (buildingIndex % storyVariantCount);
}

std::size_t textureIndexForBuildingIndex(int buildingIndex, std::size_t textureCount) {
    return static_cast<std::size_t>(buildingIndex) % textureCount;
}

std::vector<BuildingInstance> createBuildingLayout(std::size_t textureCount) {
    std::vector<BuildingInstance> buildings;
    buildings.reserve(BUILDING_COUNT);

    const float xOffset = TRACK_RADIUS_X + BUILDING_SIDE_CLEARANCE + (BUILDING_WIDTH * 0.5f);
    const float zStart = -0.5f * BUILDING_SIDE_Z_SPACING * static_cast<float>(BUILDINGS_PER_SIDE - 1);

    for (int side = 0; side < BUILDING_SIDES; ++side) {
        const float sideSign = (side == 0) ? -1.0f : 1.0f;
        const float xPos = sideSign * xOffset;

        for (int slot = 0; slot < BUILDINGS_PER_SIDE; ++slot) {
            const int buildingIndex = side * BUILDINGS_PER_SIDE + slot;
            const float zPos = zStart + (static_cast<float>(slot) * BUILDING_SIDE_Z_SPACING);

            buildings.push_back({
                glm::vec3(xPos, 0.0f, zPos),
                storyCountForBuildingIndex(buildingIndex),
                textureIndexForBuildingIndex(buildingIndex, textureCount),
            });
        }
    }

    return buildings;
}

std::vector<BuildingLight> createBuildingLights(const std::vector<BuildingInstance>& buildings) {
    std::vector<BuildingLight> lights;

    const std::size_t maxLights = std::min(buildings.size(), static_cast<std::size_t>(MAX_BUILDING_LIGHTS));
    lights.reserve(maxLights);

    for (std::size_t i = 0; i < maxLights; i++) {
        BuildingLight light;
        float roofY = buildings[i].stories * BUILDING_STORY_HEIGHT;
        glm::vec3 roofAnchor = buildings[i].position + glm::vec3(0.0f, roofY + BUILDING_LIGHT_HEIGHT_OFFSET, 0.0f);
        
        // Point down and towards the road
        if (buildings[i].position.x > 0) {
            light.baseDirection = glm::normalize(glm::vec3(-1.0f, BUILDING_LIGHT_BASE_DIRECTION_Y, 0.0f));
        }
        else {
            light.baseDirection = glm::normalize(glm::vec3(1.0f, BUILDING_LIGHT_BASE_DIRECTION_Y, 0.0f));
        }

        const glm::vec3 horizontalRoadForward = glm::normalize(glm::vec3(light.baseDirection.x, 0.0f, light.baseDirection.z));
        light.basePosition = roofAnchor
                             + glm::vec3(0.0f,
                                         BUILDING_LIGHT_GIMBAL_PIVOT_HEIGHT + BUILDING_LIGHT_LAMP_CENTER_Y,
                                         0.0f)
                             + (horizontalRoadForward * (BUILDING_LIGHT_LAMP_DEPTH * BUILDING_LIGHT_LAMP_NOZZLE_X_FACTOR));

        light.color = glm::vec3(BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][0],
                                BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][1],
                                BUILDING_LIGHT_COLORS[i % BUILDING_LIGHT_COLOR_COUNT][2]);
        light.currentSwingAngle = 0.0f;
        light.swingSpeed = BUILDING_LIGHT_SWING_BASE_SPEED + (static_cast<float>(i) * BUILDING_LIGHT_SWING_SPEED_STEP);
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

std::vector<std::vector<Mesh>> createBuildingMeshes(const std::vector<unsigned int>& textureIDs) {
    std::vector<std::vector<Mesh>> meshesByTexture;
    meshesByTexture.reserve(textureIDs.size());

    for (unsigned int textureID : textureIDs) {
        std::vector<Mesh> meshesForTexture;
        meshesForTexture.reserve(BUILDING_MAX_STORIES - BUILDING_MIN_STORIES + 1);

        for (int stories = BUILDING_MIN_STORIES; stories <= BUILDING_MAX_STORIES; ++stories) {
            meshesForTexture.push_back(ObjectMeshes::createBuilding(stories, textureID));
        }

        meshesByTexture.push_back(std::move(meshesForTexture));
    }

    return meshesByTexture;
}

Mesh& meshForBuilding(std::vector<std::vector<Mesh>>& buildingMeshes, const BuildingInstance& building) {
    return buildingMeshes[building.textureIndex][building.stories - BUILDING_MIN_STORIES];
}

void drawBuildings(unsigned int shaderProgram,
                            const glm::mat4& projection,
                            const glm::mat4& view,
                            std::vector<std::vector<Mesh>>& buildingMeshes,
                            const std::vector<BuildingInstance>& buildingLayout,
                            const std::vector<BuildingLight>& buildingLights,
                            Mesh& windmillMesh,
                            Mesh& lightGimbalBaseMesh,
                            Mesh& lightGimbalHeadMesh,
                            float currentWindmillAngle,
                            float currentTime) {
    for (std::size_t buildingIndex = 0; buildingIndex < buildingLayout.size(); ++buildingIndex) {
        const BuildingInstance& building = buildingLayout[buildingIndex];

        // draw building
        glm::mat4 buildingTransform = glm::mat4(1.0f);
        buildingTransform = glm::translate(buildingTransform, building.position);

        ObjectRenderer::drawBuilding(shaderProgram,
                                     projection,
                                     view,
                                     meshForBuilding(buildingMeshes, building),
                                     buildingTransform,
                                     glm::vec3(BUILDING_COLOR_R, BUILDING_COLOR_G, BUILDING_COLOR_B));

        float buildingHeight = building.stories * BUILDING_STORY_HEIGHT;

        if (buildingIndex < buildingLights.size()) {
            const BuildingLight& light = buildingLights[buildingIndex];

            glm::mat4 gimbalBaseTransform = buildingTransform;
            gimbalBaseTransform = glm::translate(gimbalBaseTransform,
                                                 glm::vec3(0.0f, buildingHeight + BUILDING_LIGHT_HEIGHT_OFFSET, 0.0f));

            const float roadFacingYaw = (building.position.x > 0.0f)
                ? glm::radians(BUILDING_LIGHT_ROAD_FACING_YAW_POSITIVE_X_DEG)
                : glm::radians(BUILDING_LIGHT_ROAD_FACING_YAW_NEGATIVE_X_DEG);
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
        
        // draw widnmill: start with the building's transform, then move to the top
        glm::mat4 windmillTransform = buildingTransform; 
        windmillTransform = glm::translate(windmillTransform, glm::vec3(0.0f, buildingHeight, 0.0f));

        // if the building is on the +X side, the road is to its left (-X).
        // if the building is on the -X side, the road is to its right (+X).
        if (building.position.x > 0) {
            windmillTransform = glm::translate(windmillTransform, glm::vec3(-BUILDING_WIDTH / 2.0f, 0.0f, 0.0f));
            windmillTransform = glm::rotate(windmillTransform, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else {
            windmillTransform = glm::translate(windmillTransform, glm::vec3(BUILDING_WIDTH / 2.0f, 0.0f, 0.0f));
            windmillTransform = glm::rotate(windmillTransform, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        windmillTransform = glm::rotate(windmillTransform, currentWindmillAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        // can draw any colored mesh
        ObjectRenderer::drawBuilding(shaderProgram, projection, view, 
                                     windmillMesh, 
                                     windmillTransform, 
                                     glm::vec3(WINDMILL_COLOR_R, WINDMILL_COLOR_G, WINDMILL_COLOR_B));
    }
}

void setupLighting(unsigned int shaderProgram, std::vector<BuildingLight>& lights, const glm::vec3& viewPos, float time) {
    glUseProgram(shaderProgram);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform1f(glGetUniformLocation(shaderProgram, "globalAmbientStrength"), GLOBAL_AMBIENT_STRENGTH);
    glUniform1f(glGetUniformLocation(shaderProgram, "spotlightAmbientStrength"), SPOTLIGHT_AMBIENT_STRENGTH);

    const int activeLightCount = std::min(static_cast<int>(lights.size()), MAX_BUILDING_LIGHTS);
    glUniform1i(glGetUniformLocation(shaderProgram, "numBuildingLights"), activeLightCount);

    for (int i = 0; i < activeLightCount; i++) {
        std::string prefix = "buildingLights[" + std::to_string(i) + "].";

        lights[i].currentSwingAngle = swingAngleForLight(lights[i], time);
        glm::vec3 currentDir = directionForLight(lights[i], time);

        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "position").c_str()), 1, glm::value_ptr(lights[i].basePosition));
        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "direction").c_str()), 1, glm::value_ptr(currentDir));
        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "color").c_str()), 1, glm::value_ptr(lights[i].color));
        
        // Spotlight cone geometry
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "cutOff").c_str()), glm::cos(glm::radians(BUILDING_LIGHT_INNER_CUTOFF_DEG)));
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "outerCutOff").c_str()), glm::cos(glm::radians(BUILDING_LIGHT_OUTER_CUTOFF_DEG)));
        
        // Attenuation configuration for realistic falloff
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "constant").c_str()), BUILDING_LIGHT_ATTENUATION_CONSTANT);
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "linear").c_str()), BUILDING_LIGHT_ATTENUATION_LINEAR);
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "quadratic").c_str()), BUILDING_LIGHT_ATTENUATION_QUADRATIC);
    }
}

}  // namespace

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) camera.setMode(CameraMode::SKY_VIEW);
        if (key == GLFW_KEY_2) camera.setMode(CameraMode::CAR_VIEW);
        if (key == GLFW_KEY_3) camera.setMode(CameraMode::GROUND_VIEW);
        if (key == GLFW_KEY_4) camera.setMode(CameraMode::LIGHTSOURCE_VIEW);
        if (key == GLFW_KEY_5) camera.setMode(CameraMode::HELICOPTER_CAM);
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
    std::vector<unsigned int> buildingTextures = {brickTexture, woodTexture};
    
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

    std::vector<std::vector<Mesh>> buildingMeshes = createBuildingMeshes(buildingTextures);
    std::vector<BuildingInstance> buildingLayout = createBuildingLayout(buildingTextures.size());
    std::vector<BuildingLight> buildingLights = createBuildingLights(buildingLayout);

    Mesh windmillMesh = ObjectMeshes::createWindmill();
    Mesh lightGimbalBaseMesh = ObjectMeshes::createLightGimbalBase();
    Mesh lightGimbalHeadMesh = ObjectMeshes::createLightGimbalHead();

    ObjectRenderer::CarAppearance carAppearance = ObjectRenderer::defaultCarAppearance();

    glm::vec3 groundCamPos(0.0f);
    if (!buildingLayout.empty()) {
        const BuildingInstance& b = buildingLayout[0];
        // if the building is on the -X side, the road is at +X. 
        // move the camera to the front face of the building facing the road.
        float roadDirectionX = (b.position.x < 0) ? 1.0f : -1.0f; 
        groundCamPos = b.position + glm::vec3(roadDirectionX * (BUILDING_WIDTH / 2.0f + 0.5f), CAMERA_GROUND_HEIGHT, 0.0f);
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

        glm::vec3 carPos(carX, 0.0f, carZ);
        glm::vec3 activeLightDir(DEFAULT_LIGHTSOURCE_VIEW_DIR_X,
                     DEFAULT_LIGHTSOURCE_VIEW_DIR_Y,
                     DEFAULT_LIGHTSOURCE_VIEW_DIR_Z);
        glm::vec3 activeLightPos(0.0f, DEFAULT_LIGHTSOURCE_VIEW_HEIGHT, 0.0f);
        if (!buildingLights.empty()) {
            activeLightDir = directionForLight(buildingLights[0], currentFrame);
            activeLightPos = buildingLights[0].basePosition;
        }

        glm::mat4 view = camera.getViewMatrix(carPos, carAngle, groundCamPos, activeLightPos, activeLightDir);
        glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);

        setupLighting(shaderProgram, buildingLights, cameraPos, currentFrame);

        
        processInput(window, deltaTime);
        windmillAngle += windmillSpeed * deltaTime;
        if (windmillAngle > glm::two_pi<float>()) windmillAngle -= glm::two_pi<float>();
        if (windmillAngle < -glm::two_pi<float>()) windmillAngle += glm::two_pi<float>();

        carX += carSpeed * std::cos(carAngle) * deltaTime;
        carZ += carSpeed * -std::sin(carAngle) * deltaTime; 

        glm::mat4 carModel = glm::mat4(1.0f);
        carModel = glm::translate(carModel, glm::vec3(carX, 0.0f, carZ));
        carModel = glm::rotate(carModel, carAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        carModel = glm::scale(carModel, glm::vec3(CAR_SCALE, CAR_SCALE, CAR_SCALE));

        ObjectRenderer::drawFloor(shaderProgram, projection, view, track, ground);
        ObjectRenderer::drawCar(shaderProgram,
                    projection,
                    view,
                    carFrame,
                    carWindows,
                    carWheels,
                    carModel,
                    carAppearance);

        drawBuildings(shaderProgram,
                       projection,
                       view,
                       buildingMeshes,
                       buildingLayout,
                       buildingLights,
                       windmillMesh,
                       lightGimbalBaseMesh,
                       lightGimbalHeadMesh,
                       windmillAngle,
                       currentFrame);
        

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
    for (std::vector<Mesh>& textureMeshGroup : buildingMeshes) {
        for (Mesh& buildingMesh : textureMeshGroup) {
            buildingMesh.cleanup();
        }
    }
    glfwTerminate();
    return 0;
}