#pragma once

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
};

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(
        glm::vec3 position = glm::vec3(0.0f, 2.0f, 6.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = -8.0f)
        : Position(position),
          Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          WorldUp(up),
          Yaw(yaw),
          Pitch(pitch),
          MovementSpeed(12.0f),
          MouseSensitivity(0.09f),
          Zoom(55.0f) {
        updateVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(CameraMovement direction, float deltaTime) {
        const float velocity = MovementSpeed * deltaTime;
        if (direction == CameraMovement::Forward) {
            Position += Front * velocity;
        }
        if (direction == CameraMovement::Backward) {
            Position -= Front * velocity;
        }
        if (direction == CameraMovement::Left) {
            Position -= Right * velocity;
        }
        if (direction == CameraMovement::Right) {
            Position += Right * velocity;
        }
        if (direction == CameraMovement::Up) {
            Position += WorldUp * velocity;
        }
        if (direction == CameraMovement::Down) {
            Position -= WorldUp * velocity;
        }
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            Pitch = std::clamp(Pitch, -89.0f, 89.0f);
        }

        updateVectors();
    }

    void processMouseScroll(float yoffset) {
        Zoom -= yoffset;
        Zoom = std::clamp(Zoom, 25.0f, 75.0f);
    }

private:
    void updateVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
