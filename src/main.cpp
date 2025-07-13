#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Scene.h"
#include "InputManager.h"
#include <chrono>



int main() {
    if (!glfwInit()) return -1;
    // Fenêtre modulable, dimensions fixes, non fullscreen
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    int winWidth = 1280;
    int winHeight = 720;
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Icosphere Planet", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);



    // Set viewport and projection for perfect sphere
    // Récupère la taille réelle framebuffer (pour DPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)fbWidth / (float)fbHeight;
    // Near = 10, Far = 200000 pour l'échelle planète
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 10.0f, 200000.0f);
    glLoadMatrixf(&proj[0][0]);
    glMatrixMode(GL_MODELVIEW);

    Scene scene;
    InputManager inputManager(scene);
    inputManager.setupCallbacks(window);

    auto lastTime = std::chrono::high_resolution_clock::now();
    bool wireframe = false;
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

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene.update(dt);

        // ...

        glLoadMatrixf(&scene.getCamera().getViewMatrix()[0][0]);
        scene.render(wireframe);



        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
