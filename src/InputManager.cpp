#include "InputManager.h"
Scene* InputManager::currentScene = nullptr;
bool InputManager::rotating = false;
double InputManager::lastX = 0.0;
double InputManager::lastY = 0.0;
bool InputManager::cameraControlEnabled = true;

InputManager::InputManager(Scene& scene) {
    currentScene = &scene;
}

void InputManager::setupCallbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
}

void InputManager::removeCallbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, nullptr);
    glfwSetCursorPosCallback(window, nullptr);
    glfwSetScrollCallback(window, nullptr);
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Seul le clic du milieu contrôle la caméra, le clic gauche est ignoré
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (!cameraControlEnabled) return;
        if (action == GLFW_PRESS) {
            rotating = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            rotating = false;
        }
    }
}

void InputManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (rotating && currentScene && cameraControlEnabled) {
        float dx = static_cast<float>(xpos - lastX);
        float dy = static_cast<float>(ypos - lastY);
        currentScene->getCamera().processMouseMovement(dx, -dy);
        lastX = xpos;
        lastY = ypos;
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentScene) {
        currentScene->getCamera().processMouseScroll(static_cast<float>(yoffset));
    }
}

// Méthode pour activer/désactiver le contrôle caméra
void InputManager::setCameraControlEnabled(bool enabled) {
    cameraControlEnabled = enabled;
    if (!enabled) {
        rotating = false;
    }
}
