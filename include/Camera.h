#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    // Camera vectors
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;
    
    // Euler angles
    float m_yaw;
    float m_pitch;
    
    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;
    float m_speedMultiplier; // For collision system
    
    // Mouse tracking
    bool m_firstMouse;
    float m_lastX;
    float m_lastY;
    
    void updateCameraVectors();

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = -90.0f, float pitch = 0.0f);
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    
    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    void processMouseScroll(float yoffset);
    
    // Getters
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }
    float getZoom() const { return m_zoom; }
    float getSpeedMultiplier() const { return m_speedMultiplier; }
    
    // Setters for collision system
    void setPosition(const glm::vec3& position) { m_position = position; }
    void setSpeedMultiplier(float multiplier) { m_speedMultiplier = multiplier; }
    void adjustSpeedBasedOnDistance(float nearestObjectDistance);
    
    // Camera movement directions
    enum Movement {
        FORWARD = 0,
        BACKWARD = 1,
        LEFT = 2,
        RIGHT = 3,
        UP = 4,
        DOWN = 5
    };
};
