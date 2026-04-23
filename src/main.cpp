#include <array>
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
constexpr int kBlurPasses = 10;

constexpr int kMaterialDefault = 0;
constexpr int kMaterialRoad = 1;

Camera gCamera;
float gDeltaTime = 0.0f;
float gLastFrame = 0.0f;
bool gFirstMouse = true;
float gLastX = static_cast<float>(kWindowWidth) * 0.5f;
float gLastY = static_cast<float>(kWindowHeight) * 0.5f;

struct BloomTargets {
    unsigned int hdrFBO = 0;
    std::array<unsigned int, 2> colorBuffers{};
    unsigned int depthRBO = 0;
    std::array<unsigned int, 2> pingpongFBOs{};
    std::array<unsigned int, 2> pingpongColorBuffers{};
    int width = 0;
    int height = 0;
};

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

void checkFramebufferComplete(unsigned int framebuffer, const char* label) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(std::string("Framebuffer is incomplete: ") + label);
    }
}

void destroyBloomTargets(BloomTargets& targets) {
    if (targets.depthRBO != 0) {
        glDeleteRenderbuffers(1, &targets.depthRBO);
        targets.depthRBO = 0;
    }
    if (targets.hdrFBO != 0) {
        glDeleteFramebuffers(1, &targets.hdrFBO);
        targets.hdrFBO = 0;
    }
    glDeleteTextures(static_cast<GLsizei>(targets.colorBuffers.size()), targets.colorBuffers.data());
    glDeleteTextures(static_cast<GLsizei>(targets.pingpongColorBuffers.size()), targets.pingpongColorBuffers.data());
    glDeleteFramebuffers(static_cast<GLsizei>(targets.pingpongFBOs.size()), targets.pingpongFBOs.data());
    targets.colorBuffers.fill(0);
    targets.pingpongColorBuffers.fill(0);
    targets.pingpongFBOs.fill(0);
    targets.width = 0;
    targets.height = 0;
}

void createBloomTargets(BloomTargets& targets, int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Framebuffer size must be positive.");
    }

    destroyBloomTargets(targets);
    targets.width = width;
    targets.height = height;

    glGenFramebuffers(1, &targets.hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, targets.hdrFBO);

    glGenTextures(static_cast<GLsizei>(targets.colorBuffers.size()), targets.colorBuffers.data());
    for (size_t i = 0; i < targets.colorBuffers.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, targets.colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), GL_TEXTURE_2D, targets.colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &targets.depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, targets.depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, targets.depthRBO);

    const unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    checkFramebufferComplete(targets.hdrFBO, "hdr");

    glGenFramebuffers(static_cast<GLsizei>(targets.pingpongFBOs.size()), targets.pingpongFBOs.data());
    glGenTextures(static_cast<GLsizei>(targets.pingpongColorBuffers.size()), targets.pingpongColorBuffers.data());
    for (size_t i = 0; i < targets.pingpongFBOs.size(); ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, targets.pingpongFBOs[i]);
        glBindTexture(GL_TEXTURE_2D, targets.pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targets.pingpongColorBuffers[i], 0);
        checkFramebufferComplete(targets.pingpongFBOs[i], "pingpong");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

unsigned int createScreenQuadVAO() {
    const float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
    };

    unsigned int vao = 0;
    unsigned int vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), static_cast<void*>(nullptr));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return vao;
}

void drawScreenQuad(unsigned int quadVAO) {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawCube(
    const Shader& shader,
    unsigned int vao,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& baseColor,
    const glm::vec3& emissiveColor = glm::vec3(0.0f),
    float emissiveStrength = 0.0f,
    int materialType = kMaterialDefault) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setVec3("baseColor", baseColor);
    shader.setVec3("emissiveColor", emissiveColor);
    shader.setFloat("emissiveStrength", emissiveStrength);
    shader.setInt("materialType", materialType);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawRoadAndBuildings(const Shader& shader, unsigned int cubeVAO, float timeSeconds) {
    const int currentSegment = std::max(0, static_cast<int>(std::floor(-gCamera.Position.z / kSegmentLength)) - 1);
    const int visibleSegments = 32;
    const glm::vec3 roadBaseColor(0.025f, 0.035f, 0.06f);
    const glm::vec3 laneGlowColor(0.95f, 0.45f, 0.18f);
    const glm::vec3 cyanAccent(0.08f, 0.95f, 1.0f);
    const glm::vec3 magentaAccent(1.0f, 0.18f, 0.58f);
    const glm::vec3 amberAccent(1.0f, 0.62f, 0.18f);
    const glm::vec3 buildingDarkA(0.04f, 0.06f, 0.10f);
    const glm::vec3 buildingDarkB(0.08f, 0.11f, 0.16f);
    const glm::vec3 buildingDarkC(0.06f, 0.05f, 0.10f);

    for (int segment = currentSegment; segment < currentSegment + visibleSegments; ++segment) {
        const float centerZ = -segment * kSegmentLength - kSegmentLength * 0.5f;
        const float segmentPulse = 0.65f + 0.35f * std::sin(timeSeconds * 1.1f + segment * 0.45f);

        drawCube(
            shader,
            cubeVAO,
            glm::vec3(0.0f, -0.15f, centerZ),
            glm::vec3(kRoadWidth, 0.1f, kSegmentLength),
            roadBaseColor,
            glm::vec3(0.0f),
            0.0f,
            kMaterialRoad);

        for (int side = -1; side <= 1; side += 2) {
            const glm::vec3 edgeColor = side < 0 ? cyanAccent : magentaAccent;
            drawCube(
                shader,
                cubeVAO,
                glm::vec3(side * (kRoadWidth * 0.5f - 0.35f), -0.07f, centerZ),
                glm::vec3(0.08f, 0.03f, kSegmentLength * 0.96f),
                glm::vec3(0.02f, 0.02f, 0.03f),
                edgeColor,
                1.2f + segmentPulse * 0.9f);
        }

        for (int marker = 0; marker < 4; ++marker) {
            const float markerZ = centerZ - 6.0f + marker * 4.0f;
            drawCube(
                shader,
                cubeVAO,
                glm::vec3(0.0f, -0.04f, markerZ),
                glm::vec3(0.16f, 0.03f, 1.2f),
                glm::vec3(0.05f, 0.05f, 0.05f),
                laneGlowColor,
                1.4f + 0.7f * segmentPulse);
        }

        for (int side = -1; side <= 1; side += 2) {
            for (int lot = 0; lot < 3; ++lot) {
                const int seed = segment * 19 + lot * 7 + side * 13;
                const float width = 2.0f + hash1(seed) * 1.8f;
                const float height = 7.0f + std::pow(hash1(seed + 1), 1.35f) * 19.0f;
                const float depth = 2.4f + hash1(seed + 2) * 2.5f;
                const float offsetX = side * (7.0f + lot * 3.8f + width * 0.45f);
                const float offsetZ = centerZ + (lot - 1) * 4.3f;

                glm::vec3 buildingColor = hashColor(seed + 3, buildingDarkA, buildingDarkB);
                if (hash1(seed + 31) > 0.6f) {
                    buildingColor = hashColor(seed + 9, buildingDarkA, buildingDarkC);
                }

                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(offsetX, height * 0.5f, offsetZ),
                    glm::vec3(width, height, depth),
                    buildingColor);

                if (height > 12.0f) {
                    drawCube(
                        shader,
                        cubeVAO,
                        glm::vec3(offsetX, height * 0.82f, offsetZ),
                        glm::vec3(width * 0.8f, height * 0.18f, depth * 0.8f),
                        buildingColor * 1.08f);
                }

                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(offsetX, height + 0.2f, offsetZ),
                    glm::vec3(width * 0.78f, 0.16f, depth * 0.78f),
                    glm::vec3(0.10f, 0.12f, 0.16f),
                    side < 0 ? cyanAccent : magentaAccent,
                    0.55f + 0.25f * segmentPulse);

                glm::vec3 signColor = side < 0 ? cyanAccent : magentaAccent;
                if (hash1(seed + 4) > 0.72f) {
                    signColor = amberAccent;
                }

                const float pulse = 1.5f + 0.8f * sinf(timeSeconds * 2.2f + seed);
                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(offsetX, height * 0.72f, offsetZ + depth * 0.52f),
                    glm::vec3(width * 0.65f, 0.35f, 0.08f),
                    glm::vec3(0.02f, 0.02f, 0.02f),
                    signColor,
                    pulse);

                if (hash1(seed + 15) > 0.45f) {
                    const float finX = offsetX + side * (width * 0.48f);
                    drawCube(
                        shader,
                        cubeVAO,
                        glm::vec3(finX, height * 0.52f, offsetZ),
                        glm::vec3(0.10f, height * 0.82f, depth * 0.92f),
                        glm::vec3(0.03f, 0.03f, 0.04f),
                        signColor,
                        0.65f + 0.35f * segmentPulse);
                }

                const glm::vec3 leftWindowColor = side < 0 ? cyanAccent : amberAccent;
                const glm::vec3 rightWindowColor = side < 0 ? amberAccent : magentaAccent;
                const int windowRows = std::min(8, std::max(3, static_cast<int>(height / 2.3f)));
                for (int windowRow = 0; windowRow < windowRows; ++windowRow) {
                    const float winY = 1.3f + windowRow * 2.2f;
                    if (winY > height - 1.0f) {
                        break;
                    }
                    if (hash1(seed + 70 + windowRow * 5) > 0.82f) {
                        continue;
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
                        leftWindowColor,
                        rowGlow);

                    drawCube(
                        shader,
                        cubeVAO,
                        glm::vec3(rightOffset, winY, winZ),
                        glm::vec3(0.35f, 0.5f, 0.05f),
                        glm::vec3(0.03f, 0.03f, 0.03f),
                        rightWindowColor,
                        rowGlow * 0.9f);
                }
            }

            if (segment % 5 == 0) {
                const int towerSeed = segment * 41 + side * 23;
                const float towerHeight = 24.0f + hash1(towerSeed) * 26.0f;
                const float towerWidth = 2.6f + hash1(towerSeed + 1) * 1.6f;
                const float towerDepth = 2.2f + hash1(towerSeed + 2) * 1.4f;
                const float towerX = side * (17.0f + hash1(towerSeed + 3) * 5.0f);
                const float towerZ = centerZ + (hash1(towerSeed + 4) - 0.5f) * 7.0f;
                const glm::vec3 towerColor = hashColor(towerSeed + 5, buildingDarkA, buildingDarkC);
                const glm::vec3 towerAccent = side < 0 ? cyanAccent : magentaAccent;

                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(towerX, towerHeight * 0.5f, towerZ),
                    glm::vec3(towerWidth, towerHeight, towerDepth),
                    towerColor);

                drawCube(
                    shader,
                    cubeVAO,
                    glm::vec3(towerX, towerHeight + 0.6f, towerZ),
                    glm::vec3(0.22f, 1.1f, 0.22f),
                    glm::vec3(0.04f, 0.04f, 0.06f),
                    towerAccent,
                    1.4f + 0.6f * segmentPulse);
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

        Shader sceneShader(
            APP_SHADER_DIR "basic.vert",
            APP_SHADER_DIR "basic.frag");
        Shader blurShader(
            APP_SHADER_DIR "screen.vert",
            APP_SHADER_DIR "blur.frag");
        Shader compositeShader(
            APP_SHADER_DIR "screen.vert",
            APP_SHADER_DIR "composite.frag");

        const unsigned int cubeVAO = createCubeVAO();
        const unsigned int quadVAO = createScreenQuadVAO();
        BloomTargets bloomTargets;

        blurShader.use();
        blurShader.setInt("image", 0);

        compositeShader.use();
        compositeShader.setInt("sceneTexture", 0);
        compositeShader.setInt("bloomBlurTexture", 1);

        while (!glfwWindowShouldClose(window)) {
            const float currentFrame = static_cast<float>(glfwGetTime());
            gDeltaTime = currentFrame - gLastFrame;
            gLastFrame = currentFrame;

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            if (framebufferWidth <= 0 || framebufferHeight <= 0) {
                glfwPollEvents();
                continue;
            }
            if (bloomTargets.width != framebufferWidth || bloomTargets.height != framebufferHeight) {
                createBloomTargets(bloomTargets, framebufferWidth, framebufferHeight);
            }

            processInput(window);

            glBindFramebuffer(GL_FRAMEBUFFER, bloomTargets.hdrFBO);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            sceneShader.use();

            const float aspectRatio =
                framebufferHeight > 0 ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 16.0f / 9.0f;

            const glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), aspectRatio, 0.1f, 500.0f);
            const glm::mat4 view = gCamera.getViewMatrix();

            sceneShader.setMat4("projection", projection);
            sceneShader.setMat4("view", view);
            sceneShader.setVec3("cameraPos", gCamera.Position);
            sceneShader.setVec3("fogColor", glm::vec3(0.015f, 0.030f, 0.065f));
            sceneShader.setFloat("fogDensity", 0.024f);
            sceneShader.setFloat("bloomThreshold", 1.0f);
            sceneShader.setFloat("timeSeconds", currentFrame);

            drawRoadAndBuildings(sceneShader, cubeVAO, currentFrame);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);

            bool horizontal = true;
            bool firstIteration = true;
            blurShader.use();
            for (int i = 0; i < kBlurPasses; ++i) {
                glBindFramebuffer(GL_FRAMEBUFFER, bloomTargets.pingpongFBOs[horizontal ? 1 : 0]);
                blurShader.setBool("horizontal", horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(
                    GL_TEXTURE_2D,
                    firstIteration ? bloomTargets.colorBuffers[1] : bloomTargets.pingpongColorBuffers[horizontal ? 0 : 1]);
                drawScreenQuad(quadVAO);
                horizontal = !horizontal;
                if (firstIteration) {
                    firstIteration = false;
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            compositeShader.use();
            compositeShader.setFloat("exposure", 1.02f);
            compositeShader.setFloat("bloomStrength", 0.82f);
            compositeShader.setFloat("timeSeconds", currentFrame);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomTargets.colorBuffers[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, bloomTargets.pingpongColorBuffers[horizontal ? 0 : 1]);
            drawScreenQuad(quadVAO);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        destroyBloomTargets(bloomTargets);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
