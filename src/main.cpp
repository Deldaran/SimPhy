#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Scene.h"
#include "InputManager.h"
#include <chrono>



int main() {
    if (!glfwInit()) return -1;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Icosphere Planet", monitor, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewInit();



    // Set viewport and projection for perfect sphere
    int winWidth, winHeight;
    glfwGetFramebufferSize(window, &winWidth, &winHeight);
    glViewport(0, 0, winWidth, winHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)winWidth / (float)winHeight;
    // Near = 10, Far = 200000 pour l'échelle planète
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 10.0f, 200000.0f);
    glLoadMatrixf(&proj[0][0]);
    glMatrixMode(GL_MODELVIEW);

    Scene scene;
    InputManager inputManager(scene);
    inputManager.setupCallbacks(window);

    auto lastTime = std::chrono::high_resolution_clock::now();
    bool wireframe = false;
    bool useRaymarch = false;
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        glfwPollEvents();
        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);
        glViewport(0, 0, winWidth, winHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)winWidth / (float)winHeight;
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 10.0f, 200000.0f);
        glLoadMatrixf(&proj[0][0]);
        glMatrixMode(GL_MODELVIEW);
        // ...



        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            wireframe = !wireframe;
            while (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
                glfwPollEvents();
            }
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            useRaymarch = !useRaymarch;
            while (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                glfwPollEvents();
            }
        }

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene.update(dt);

        // ...

        glLoadMatrixf(&scene.getCamera().getViewMatrix()[0][0]);
        scene.render(wireframe, useRaymarch);



        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
