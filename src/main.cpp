#include <algorithm>

// Utilitaire clamp compatible C++14
template<typename T>
T clamp(T val, T minVal, T maxVal) {
    return std::max(minVal, std::min(val, maxVal));
}
// ...existing code...
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Icosphere.h"
#include "Camera.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>



int compileShader(const std::string& src, GLenum type) {
    int shader = glCreateShader(type);
    const char* csrc = src.c_str();
    printf("Shader source (type %d):\n%s\n", type, src.c_str());
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

int createProgram(const std::string& vertPath, const std::string& tescPath, const std::string& tesePath, const std::string& fragPath) {
    auto readFile = [](const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            printf("Erreur ouverture fichier shader: %s\n", path.c_str());
        } else {
            printf("Ouverture OK fichier shader: %s\n", path.c_str());
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    int vert = compileShader(readFile(vertPath), GL_VERTEX_SHADER);
    int tesc = compileShader(readFile(tescPath), GL_TESS_CONTROL_SHADER);
    int tese = compileShader(readFile(tesePath), GL_TESS_EVALUATION_SHADER);
    int frag = compileShader(readFile(fragPath), GL_FRAGMENT_SHADER);
    int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, tesc);
    glAttachShader(prog, tese);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(tesc);
    glDeleteShader(tese);
    glDeleteShader(frag);
    return prog;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    int winWidth = 1280;
    int winHeight = 720;
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Icosphere Planet", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Callback pour le scroll (zoom) - doit être après la création de la fenêtre
    struct ScrollData { float yoffset = 0.0f; } scrollData;
    auto scrollCallback = [](GLFWwindow* w, double xoffset, double yoffset) {
        auto* sd = (ScrollData*)glfwGetWindowUserPointer(w);
        sd->yoffset += (float)yoffset;
    };
    glfwSetWindowUserPointer(window, &scrollData);
    glfwSetScrollCallback(window, scrollCallback);

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported: %s\n", version);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Création de l'icosphere et des buffers
    Icosphere icosphere(glm::vec3(0.0f), 6371.0f, 5);
    icosphere.initGLBuffers();

    // Chargement des shaders
    auto readFile = [](const std::string& path) {
        std::ifstream f(path);
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    int vert = compileShader(readFile("planet.vert"), GL_VERTEX_SHADER);
    int tesc = compileShader(readFile("planet.tesc"), GL_TESS_CONTROL_SHADER);
    int tese = compileShader(readFile("planet.tese"), GL_TESS_EVALUATION_SHADER);
    int frag = compileShader(readFile("planet.frag"), GL_FRAGMENT_SHADER);
    int planetProgram = glCreateProgram();
    glAttachShader(planetProgram, vert);
    glAttachShader(planetProgram, tesc);
    glAttachShader(planetProgram, tese);
    glAttachShader(planetProgram, frag);
    glLinkProgram(planetProgram);
    glDeleteShader(vert);
    glDeleteShader(tesc);
    glDeleteShader(tese);
    glDeleteShader(frag);

    const float planetRadius = 6371.0f;
    Camera camera(planetRadius * 2.0f, 0.0f, 0.0f); // Distance initiale = 2x rayon planète
    bool orbitMode = true; // true = orbite, false = avion
    glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
    float baseSpeed = 0.2f; // vitesse relative
    float cameraSpeed = baseSpeed * camera.radius; // vitesse dépend du rayon
    double lastX = 0, lastY = 0;
    bool firstMouse = true;
    glm::mat4 model = glm::mat4(1.0f);
    auto lastTime = std::chrono::high_resolution_clock::now();
    bool wireframe = false;
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        glfwPollEvents();
        // Wireframe toggle
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            wireframe = !wireframe;
            while (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) glfwPollEvents();
        }
        // Mode switch
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            orbitMode = !orbitMode;
            while (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) glfwPollEvents();
        }
        // Mouse movement (rotation seulement si clic gauche)
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float dx = 0, dy = 0;
        if (leftPressed) {
            if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
            dx = float(xpos - lastX);
            dy = float(lastY - ypos);
        }
        lastX = xpos; lastY = ypos;
        if (orbitMode) {
            if (leftPressed) camera.processMouseMovement(dx, dy);
        } else {
            if (leftPressed) {
                camera.yaw += dx * 0.2f;
                camera.pitch += dy * 0.2f;
                camera.pitch = clamp(camera.pitch, -89.0f, 89.0f);
            }
            float radYaw = glm::radians(camera.yaw);
            float radPitch = glm::radians(camera.pitch);
            cameraFront.x = cos(radPitch) * sin(radYaw);
            cameraFront.y = sin(radPitch);
            cameraFront.z = cos(radPitch) * cos(radYaw);
            cameraFront = glm::normalize(cameraFront);
        }
        // Zoom/dézoom avec la molette
        if (scrollData.yoffset != 0.0f) {
            float zoomFactor = 0.1f * planetRadius; // Zoom proportionnel au rayon planète
            camera.radius -= scrollData.yoffset * zoomFactor;
            camera.radius = clamp(camera.radius, planetRadius * 1.1f, planetRadius * 20.0f); // Jamais dans la planète
            camera.updatePosition();
            scrollData.yoffset = 0.0f;
        }
        // Ajuste la vitesse à chaque frame (si le rayon change)
        cameraSpeed = baseSpeed * camera.radius;
        // Scroll (zoom/orbit radius)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.processMouseScroll(-0.01f);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.processMouseScroll(0.01f);
        // Déplacement clavier
        if (!orbitMode) {
            glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.position += cameraFront * cameraSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.position -= cameraFront * cameraSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.position -= right * cameraSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.position += right * cameraSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.position += cameraUp * cameraSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.position -= cameraUp * cameraSpeed * dt;
        }
        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);
        glViewport(0, 0, winWidth, winHeight);
        float aspect = (float)winWidth / (float)winHeight;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 10.0f, 200000000.0f);
        glm::mat4 view;
        if (orbitMode) {
            view = camera.getViewMatrix();
        } else {
            view = glm::lookAt(camera.position, camera.position + cameraFront, cameraUp);
        }

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); // Désactive le face culling

        glUseProgram(planetProgram);
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        // Ajout des uniforms pour le shader procédural
        glUniform1f(glGetUniformLocation(planetProgram, "planetRadius"), planetRadius);
        glUniform3f(glGetUniformLocation(planetProgram, "planetCenter"), 0.0f, 0.0f, 0.0f);

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        icosphere.draw();

        glfwSwapBuffers(window);
    }
    icosphere.cleanupGLBuffers();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
