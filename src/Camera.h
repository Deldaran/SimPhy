#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class Camera {
public:
    // Constructeur orbite (autour d'un point)
    Camera(float radius, float yaw, float pitch, const glm::vec3& target);

    void setOrbitMode(bool orbit, const glm::vec3& newTarget);

    void processMouseMovement(float dx, float dy);
    void processMouseScroll(float dy);
    void processKeyboard(const std::string& move, float deltaTime, float speed);
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const;
    void updatePosition();

    // Membres principaux
    // Mode orbite uniquement
    float radius = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 target = glm::vec3(0.0f);
};
