#include "Camera.h"
#include <algorithm>

Camera::Camera(float radius, float yaw, float pitch)
    : radius(radius), yaw(yaw), pitch(pitch) {
    updatePosition();
}

void Camera::processMouseMovement(float dx, float dy) {
    yaw += dx * 0.2f;
    pitch += dy * 0.2f;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
    updatePosition();
}

void Camera::processMouseScroll(float dy) {
    radius -= dy * 0.5f;
    radius = std::max(radius, 1.0f);
    updatePosition();
}

void Camera::updatePosition() {
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    position.x = radius * cos(radPitch) * sin(radYaw);
    position.y = radius * sin(radPitch);
    position.z = radius * cos(radPitch) * cos(radYaw);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getPosition() const {
    return position;
}
