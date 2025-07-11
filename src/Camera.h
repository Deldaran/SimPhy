#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float radius = 5.0f, float yaw = 0.0f, float pitch = 0.0f);
    void processMouseMovement(float dx, float dy);
    void processMouseScroll(float dy);
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const;
    float getRadius() const { return radius; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
private:
    float radius;
    float yaw;
    float pitch;
    glm::vec3 position;
    void updatePosition();
};
