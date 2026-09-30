#include "Camera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>

void Camera::SetMode(GLFWwindow* window, Mode newMode) {
    mode = newMode;
    firstMouse = true;
    glfwSetInputMode(window, GLFW_CURSOR, mode == Mode::Free ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Camera::ResetView() {
    center = glm::vec2(0.0f);
    halfHeight = 25.0f;
    position = glm::vec3(0.0f, -45.0f, 30.0f);
    yaw = 90.0f;
    pitch = -33.7f;
}

void Camera::Update(GLFWwindow* window, float realDt) {
    if (mode == Mode::TopDown)
        UpdateTopDown(window, realDt);
    else
        UpdateFree(window, realDt);
}

void Camera::UpdateTopDown(GLFWwindow* window, float realDt) {
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureMouse) {
        // Glisser : on convertit le déplacement en pixels en kpc pour que le disque suive la souris.
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            float kpcPerPixel = 2.0f * halfHeight / io.DisplaySize.y;
            center.x -= delta.x * kpcPerPixel;
            center.y += delta.y * kpcPerPixel;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
        if (io.MouseWheel != 0.0f)
            halfHeight = std::clamp(halfHeight * std::pow(0.9f, io.MouseWheel), 0.5f, 500.0f);
    }

    if (!io.WantCaptureKeyboard) {
        float speed = halfHeight * realDt; // une demi-hauteur d'écran par seconde
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) center.x += speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) center.x -= speed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) center.y += speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) center.y -= speed;
    }
}

void Camera::UpdateFree(GLFWwindow* window, float realDt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        SetMode(window, Mode::TopDown);
        return;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (firstMouse) {
        lastMouseX = x;
        lastMouseY = y;
        firstMouse = false;
    }
    const float sensitivity = 0.1f;
    yaw -= float(x - lastMouseX) * sensitivity;
    pitch = std::clamp(pitch - float(y - lastMouseY) * sensitivity, -89.0f, 89.0f);
    lastMouseX = x;
    lastMouseY = y;

    const glm::vec3 up(0.0f, 0.0f, 1.0f);
    glm::vec3 front = Front();
    glm::vec3 right = glm::normalize(glm::cross(front, up));

    float speed = 20.0f * realDt; // kpc par seconde réelle
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 4.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= front * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * speed;
}

glm::vec3 Camera::Front() const {
    float y = glm::radians(yaw);
    float p = glm::radians(pitch);
    return glm::vec3(std::cos(p) * std::cos(y), std::cos(p) * std::sin(y), std::sin(p));
}

glm::mat4 Camera::View() const {
    if (mode == Mode::TopDown) {
        // On regarde le plan du disque depuis +z, l'axe y vers le haut de l'écran.
        glm::vec3 eye(center, 0.0f);
        return glm::lookAt(eye, eye - glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    return glm::lookAt(position, position + Front(), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 Camera::Projection(float aspect) const {
    if (mode == Mode::TopDown) {
        float halfWidth = halfHeight * aspect;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -10000.0f, 10000.0f);
    }
    return glm::perspective(glm::radians(60.0f), aspect, 0.01f, 5000.0f);
}
