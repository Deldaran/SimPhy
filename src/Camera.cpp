
#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(float radius, float yaw, float pitch, const glm::vec3& target)
    : radius(radius), yaw(yaw), pitch(pitch), target(target) {
    updatePosition();
}




void Camera::setOrbitMode(bool /*orbit*/, const glm::vec3& newTarget) {
    // Mode orbite uniquement
    target = newTarget;
    glm::vec3 offset = position - target;
    radius = glm::length(offset);
    yaw = glm::degrees(atan2(offset.x, offset.z));
    pitch = glm::degrees(asin(offset.y / radius));
    updatePosition();
}

void Camera::processMouseMovement(float dx, float dy) {
    yaw += dx;
    pitch += dy;
    // Limite le pitch pour éviter le flip
    if (pitch > 89.9f) pitch = 89.9f;
    if (pitch < -89.9f) pitch = -89.9f;
    updatePosition();
}

void Camera::processKeyboard(const std::string& direction, float deltaTime, float speed) {
    // Mode orbite : pas de déplacement clavier
    (void)direction; (void)deltaTime; (void)speed;
}

void Camera::processMouseScroll(float dy) {
    radius -= dy * 10.0f;
    if (radius < 1.0f) radius = 1.0f;
    updatePosition();
}

void Camera::updatePosition() {
    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    position.x = target.x + radius * cos(radPitch) * sin(radYaw);
    position.y = target.y + radius * sin(radPitch);
    position.z = target.z + radius * cos(radPitch) * cos(radYaw);
    direction = glm::normalize(target - position);
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(position, target, up);
}

glm::vec3 Camera::getPosition() const {
    return position;
}
