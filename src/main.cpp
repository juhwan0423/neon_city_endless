#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "shader.h"

namespace {

constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;
constexpr float kSegmentLength = 18.0f;
constexpr float kRoadWidth = 8.0f;

Camera gCamera;
float gDeltaTime = 0.0f;
float gLastFrame = 0.0f;
bool gFirstMouse = true;
float gLastX = static_cast<float>(kWindowWidth) * 0.5f;
float gLastY = static_cast<float>(kWindowHeight) * 0.5f;

float hash1(int value) {
    const float x = sinf(static_cast<float>(value) * 12.9898f) * 43758.5453f;
    return x - floorf(x);
}

glm::vec3 hashColor(int seed, const glm::vec3& a, const glm::vec3& b) {
    return glm::mix(a, b, hash1(seed));
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow*, double xposIn, double yposIn) {
    const float xpos = static_cast<float>(xposIn);
    const float ypos = static_cast<float>(yposIn);

    if (gFirstMouse) {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    const float xoffset = xpos - gLastX;
    const float yoffset = gLastY - ypos;
    gLastX = xpos;
    gLastY = ypos;

    gCamera.processMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow*, double, double yoffset) {
    gCamera.processMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Forward, gDeltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Backward, gDeltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Left, gDeltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Right, gDeltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Down, gDeltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        gCamera.processKeyboard(CameraMovement::Up, gDeltaTime);
    }
}

unsigned int createCubeVAO() {
    const float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
    };

    unsigned int vao = 0;
    unsigned int vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return vao;
}

void drawCube(
    const Shader& shader,
    unsigned int vao,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& baseColor,
    const glm::vec3& emissiveColor = glm::vec3(0.0f),
    float emissiveStrength = 0.0f) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setVec3("baseColor", baseColor);
    shader.setVec3("emissiveColor", emissiveColor);
    shader.setFloat("emissiveStrength", emissiveStrength);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawRoadAndBuildings(const Shader& shader, unsigned int cubeVAO, float timeSeconds) {
    const int currentSegment = std::max(0, static_cast<int>(std::floor(-gCamera.Position.z / kSegmentLength)) - 1);
    const int visibleSegments = 32;

    for (int segment = currentSegment; segment < currentSegment + visibleSegments; ++segment) {
        const float centerZ = -segment * kSegmentLength - kSegmentLength * 0.5f;

        drawCube(
            shader,
            cubeVAO,
            glm::vec3(0.0f, -0.15f, centerZ),
            glm::vec3(kRoadWidth, 0.1f, kSegmentLength),
            glm::vec3(0.04f, 0.05f, 0.08f));

        for (int marker = 0; marker < 4; ++marker) {
            const float markerZ = centerZ - 6.0f + marker * 4.0f;
            drawCube(
                shader,
                cubeVAO,
                glm::vec3(0.0f, -0.04f, markerZ),
                glm::vec3(0.16f, 0.03f, 1.2f),
                glm::vec3(0.05f, 0.05f, 0.05f),
                glm::vec3(0.2f, 0.8f, 1.0f),
                1.8f);
        }

        for (int side = -1; side <= 1; side += 2) {
            for (int lot = 0; lot < 3; ++lot) {
                const int seed = segment * 19 + lot * 7 + side * 13;
                const float width = 2.0f + hash1(seed) * 1.8f;
                const float height = 5.0f + hash1(seed + 1) * 16.0f;
                const float depth = 2.4f + hash1(seed + 2) * 2.5f;
                const float offsetX = side * (7.0f + lot * 3.8f + width * 0.45f);
                const float offsetZ = centerZ + (lot - 1) * 4.3f;

                const glm::vec3 buildingColor = hashColor(
                    seed + 3,
                    glm::vec3(0.06f, 0.08f, 0.12f),
                    glm::vec3(0.12f, 0.16f, 0.22f));

                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(offsetX, height * 0.5f, offsetZ),
                    glm::vec3(width, height, depth),
                    buildingColor);

                const glm::vec3 signColor = hashColor(
                    seed + 4,
                    glm::vec3(1.0f, 0.15f, 0.55f),
                    glm::vec3(0.1f, 0.95f, 1.0f));

                const float pulse = 1.5f + 0.8f * sinf(timeSeconds * 2.2f + seed);
                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(offsetX, height * 0.72f, offsetZ + depth * 0.52f),
                    glm::vec3(width * 0.65f, 0.35f, 0.08f),
                    glm::vec3(0.02f, 0.02f, 0.02f),
                    signColor,
                    pulse);

                for (int windowRow = 0; windowRow < 4; ++windowRow) {
                    const float winY = 1.3f + windowRow * 2.2f;
                    if (winY > height - 1.0f) {
                        break;
                    }
                    const float leftOffset = offsetX - width * 0.22f;
                    const float rightOffset = offsetX + width * 0.22f;
                    const float winZ = offsetZ + depth * 0.51f;
                    const float rowGlow = 0.9f + 0.5f * hash1(seed + 20 + windowRow);

                    drawCube(
                        shader,
                        cubeVAO,
                        glm::vec3(leftOffset, winY, winZ),
                        glm::vec3(0.35f, 0.5f, 0.05f),
                        glm::vec3(0.03f, 0.03f, 0.03f),
                        glm::vec3(0.7f, 0.95f, 1.0f),
                        rowGlow);

                    drawCube(
                        shader,
                        cubeVAO,
                        glm::vec3(rightOffset, winY, winZ),
                        glm::vec3(0.35f, 0.5f, 0.05f),
                        glm::vec3(0.03f, 0.03f, 0.03f),
                        glm::vec3(0.9f, 0.25f, 0.7f),
                        rowGlow * 0.9f);
                }
            }
        }
    }
}

}  // namespace

int main() {
    try {
        if (glfwInit() == GLFW_FALSE) {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(
            static_cast<int>(kWindowWidth),
            static_cast<int>(kWindowHeight),
            "Neon City Starter",
            nullptr,
            nullptr);

        if (window == nullptr) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window.");
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLEW.");
        }

        glEnable(GL_DEPTH_TEST);

        Shader shader(
            APP_SHADER_DIR "basic.vert",
            APP_SHADER_DIR "basic.frag");

        const unsigned int cubeVAO = createCubeVAO();

        while (!glfwWindowShouldClose(window)) {
            const float currentFrame = static_cast<float>(glfwGetTime());
            gDeltaTime = currentFrame - gLastFrame;
            gLastFrame = currentFrame;

            processInput(window);

            glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            const float aspectRatio =
                framebufferHeight > 0 ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 16.0f / 9.0f;

            const glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), aspectRatio, 0.1f, 500.0f);
            const glm::mat4 view = gCamera.getViewMatrix();

            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            shader.setVec3("cameraPos", gCamera.Position);
            shader.setVec3("fogColor", glm::vec3(0.02f, 0.04f, 0.08f));
            shader.setFloat("fogDensity", 0.022f);

            drawRoadAndBuildings(shader, cubeVAO, currentFrame);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
