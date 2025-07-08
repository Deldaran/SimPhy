#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position), m_worldUp(up), m_yaw(yaw), m_pitch(pitch),
      m_movementSpeed(50.0f), m_mouseSensitivity(0.1f), m_zoom(45.0f), m_speedMultiplier(1.0f),
      m_firstMouse(true), m_lastX(400.0f), m_lastY(300.0f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_zoom), aspect, 0.1f, 50000.0f);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime * m_speedMultiplier;
    
    switch (direction) {
        case FORWARD:
            m_position += m_front * velocity;
            break;
        case BACKWARD:
            m_position -= m_front * velocity;
            break;
        case LEFT:
            m_position -= m_right * velocity;
            break;
        case RIGHT:
            m_position += m_right * velocity;
            break;
        case UP:
            m_position += m_worldUp * velocity;
            break;
        case DOWN:
            m_position -= m_worldUp * velocity;
            break;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;
    
    m_yaw += xoffset;
    m_pitch += yoffset;
    
    // Constrain pitch to prevent screen flipping
    m_pitch = std::max(-89.0f, std::min(89.0f, m_pitch));
    
    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    m_zoom -= yoffset;
    m_zoom = std::max(1.0f, std::min(45.0f, m_zoom));
}

void Camera::updateCameraVectors() {
    // Calculate new front vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    
    // Calculate right and up vectors
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::adjustSpeedBasedOnDistance(float nearestObjectDistance) {
    // Ajuste automatiquement la vitesse de déplacement selon la distance aux objets
    float baseSpeed = 50.0f;
    
    // Si on est très proche d'un objet, ralentir drastiquement
    if (nearestObjectDistance < 10.0f) {
        m_movementSpeed = baseSpeed * 0.1f;
    }
    // Si on est proche, vitesse modérée
    else if (nearestObjectDistance < 100.0f) {
        float factor = nearestObjectDistance / 100.0f;
        m_movementSpeed = baseSpeed * (0.1f + 0.4f * factor);
    }
    // Si on est à distance moyenne, vitesse normale
    else if (nearestObjectDistance < 1000.0f) {
        float factor = nearestObjectDistance / 1000.0f;
        m_movementSpeed = baseSpeed * (0.5f + 0.5f * factor);
    }
    // Si on est loin, vitesse élevée pour les grandes distances
    else {
        m_movementSpeed = baseSpeed * std::min(10.0f, nearestObjectDistance / 1000.0f);
    }
}
