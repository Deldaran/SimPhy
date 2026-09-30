#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

// Deux modes : vue de dessus orthographique (souris + clavier)
// et vol libre en perspective (souris + ZQSD/WASD).
// Le disque galactique est dans le plan (x, y), l'axe z est vertical.
class Camera {
public:
    enum class Mode { TopDown, Free };

    void Update(GLFWwindow* window, float realDt);
    void SetMode(GLFWwindow* window, Mode newMode);
    void ResetView();

    Mode GetMode() const { return mode; }
    glm::mat4 View() const;
    glm::mat4 Projection(float aspect) const;

private:
    void UpdateTopDown(GLFWwindow* window, float realDt);
    void UpdateFree(GLFWwindow* window, float realDt);
    glm::vec3 Front() const;

    Mode mode = Mode::TopDown;

    // Vue de dessus
    glm::vec2 center{ 0.0f };
    float halfHeight = 25.0f; // demi-hauteur visible (kpc)

    // Vol libre
    glm::vec3 position{ 0.0f, -45.0f, 30.0f };
    float yaw = 90.0f;    // degrés, 90 = regarde vers +y
    float pitch = -33.7f; // degrés
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool firstMouse = true;
};
