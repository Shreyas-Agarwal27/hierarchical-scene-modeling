#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

#include "constants.h"
#include "mesh.h"
#include "object_meshes.h"
#include "object_renderer.h"
#include "shader_utils.h"
#include "texture_utils.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float carSpeed = 5.0f;
float carDistance = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    ObjectRenderer::CarAppearance carAppearance = ObjectRenderer::defaultCarAppearance();

    // game loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // render
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 300.0f, 0.0f), // location
            glm::vec3(0.0f, 0.0f, 0.0f),  // look at
            glm::vec3(0.0f, 0.0f, -1.0f)  // up vector cant be (0, 1, 0) as that would be anti parallel to p-e
        );
        
        carDistance += carSpeed * deltaTime;

        float halfWidth = TRACK_WIDTH / 2.0f;
        float spineRx = TRACK_RADIUS_X - halfWidth;
        float spineRz = TRACK_RADIUS_Z - halfWidth;
        float avgRadius = (spineRx + spineRz) / 2.0f;
        float theta = carDistance / avgRadius; // Arc length formula

        float carX = spineRx * std::cos(theta);
        float carZ = spineRz * std::sin(theta);

        // Calculate rotation so car points forward
        float tx = -spineRx * std::sin(theta);
        float tz = spineRz * std::cos(theta);
        float carAngle = std::atan2(-tz, tx); 

        // Generate car transform matrix
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
        
        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    ground.cleanup();
    track.cleanup();
    carFrame.cleanup();
    carWindows.cleanup();
    carWheels.cleanup();
    glfwTerminate();
    return 0;
}