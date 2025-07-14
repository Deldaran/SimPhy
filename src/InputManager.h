#pragma once
#include "Scene.h"

class InputManager {
public:
    InputManager(Scene& scene);
    void setupCallbacks(GLFWwindow* window);
    void removeCallbacks(GLFWwindow* window);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    // Active ou désactive le contrôle caméra (rotation)
    static void setCameraControlEnabled(bool enabled);
private:
    static Scene* currentScene;
    static bool rotating;
    static double lastX, lastY;
    static bool cameraControlEnabled;
};
