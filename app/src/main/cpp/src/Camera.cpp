#include "../includes/Camera.hpp"

namespace gps {

    // Constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        // Calculate the initial Frame of Reference
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, cameraUp));
        this->cameraUpDirection = glm::cross(this->cameraRightDirection, this->cameraFrontDirection);
    }

    // Returns the View Matrix
    glm::mat4 Camera::getViewMatrix() {
        // The View Matrix transforms World Space -> View Space
        // We look from 'cameraPosition' towards 'cameraPosition + Front'
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    // Handles movement (WASD style or Automated Flythrough)
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
            case MOVE_FORWARD:
                cameraPosition += cameraFrontDirection * speed;
                break;
            case MOVE_BACKWARD:
                cameraPosition -= cameraFrontDirection * speed;
                break;
            case MOVE_RIGHT:
                cameraPosition += cameraRightDirection * speed;
                break;
            case MOVE_LEFT:
                cameraPosition -= cameraRightDirection * speed;
                break;
        }
    }

    // Handles rotation (Touch drag or Mouse look)
    void Camera::rotate(float pitch, float yaw) {
        // 1. Calculate new Front vector using Euler Angles
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        this->cameraFrontDirection = glm::normalize(front);

        // 2. Re-calculate Right and Up vectors
        // We assume the world Up is always (0, 1, 0) to prevent rolling
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, worldUp));
        this->cameraUpDirection = glm::normalize(glm::cross(this->cameraRightDirection, this->cameraFrontDirection));
    }
}