#include "Camera.h"
#include <algorithm>

Camera::Camera(float radius, float yaw, float pitch)
    : radius(radius), yaw(yaw), pitch(pitch) {
    updatePosition();
}

void Camera::processMouseMovement(float dx, float dy) {
    yaw += dx * 2.0f; // Rotation très rapide
    pitch += dy * 2.0f;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
    updatePosition();
}

void Camera::processMouseScroll(float dy) {
    radius -= dy * 100000.0f; // Zoom ultra rapide
    if (radius < 10.0f) radius = 10.0f; // Limite seulement en zoom avant
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
    // Calculer l'up vector pour éviter le flip aux pôles
    glm::vec3 up;
    float radPitch = glm::radians(pitch);
    float radYaw = glm::radians(yaw);
    // Position caméra déjà calculée
    glm::vec3 camToCenter = -glm::normalize(position);
    // Up vector = dérivée de la position par rapport à la latitude
    up.x = -cos(radPitch + glm::radians(1.0f)) * sin(radYaw);
    up.y = -sin(radPitch + glm::radians(1.0f));
    up.z = -cos(radPitch + glm::radians(1.0f)) * cos(radYaw);
    up = glm::normalize(up);
    return glm::lookAt(position, glm::vec3(0.0f), up);
}

glm::vec3 Camera::getPosition() const {
    return position;
}
