
// ...existing code...
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Icosphere.h"
#include "SunSphere.h"
#include "Camera.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>

const float planetRadius = 6371.0f;
glm::vec3 sunDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));

template<typename T>
T clamp(T val, T minVal, T maxVal) {
    return std::max(minVal, std::min(val, maxVal));
}


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

// Activation GPU dédié (NVIDIA/AMD)
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

struct AvionCamera {
    glm::vec3 position = glm::vec3(0.0f, 10000.0f, 0.0f);
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
    float speed = 100.0f;
};

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

    // Constantes planète et soleil
    // Création de l'icosphere et des buffers
    Icosphere icosphere(glm::vec3(0.0f), planetRadius, 5);
    icosphere.initGLBuffers();
    // Création du soleil (sphère lisse)
    glm::vec3 sunPos = glm::vec3(0.0f) + sunDir * planetRadius * 10.0f;
    SunSphere sun(sunPos, planetRadius * 1000.0f); // rayon 1000x planète
    sun.initGLBuffers();

    // Chargement des shaders
    auto readFile = [](const std::string& path) {
        std::ifstream f(path);
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    // Shaders planète
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
    // Shaders soleil
    int sunVert = compileShader(readFile("sun.vert"), GL_VERTEX_SHADER);
    int sunFrag = compileShader(readFile("sun.frag"), GL_FRAGMENT_SHADER);
    int sunProgram = glCreateProgram();
    glAttachShader(sunProgram, sunVert);
    glAttachShader(sunProgram, sunFrag);
    glLinkProgram(sunProgram);
    glDeleteShader(sunVert);
    glDeleteShader(sunFrag);

    // Shaders shadowmap
    int shadowVert = compileShader(readFile("shadow.vert"), GL_VERTEX_SHADER);
    int shadowFrag = compileShader(readFile("shadow.frag"), GL_FRAGMENT_SHADER);
    int shadowProgram = glCreateProgram();
    glAttachShader(shadowProgram, shadowVert);
    glAttachShader(shadowProgram, shadowFrag);
    glLinkProgram(shadowProgram);
    glDeleteShader(shadowVert);
    glDeleteShader(shadowFrag);

    const float planetRadius = 6371.0f;
    Camera camera(planetRadius * 2.0f, 0.0f, 0.0f); // Distance initiale = 2x rayon planète
    bool orbitMode = true; // true = orbite, false = avion
    bool avionMode = false;
    AvionCamera avionCam;
    glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
    float baseSpeed = 0.2f; // vitesse relative
    float cameraSpeed = baseSpeed * camera.radius; // vitesse dépend du rayon
    double lastX = 0, lastY = 0;
    bool firstMouse = true;
    glm::mat4 model = glm::mat4(1.0f);
    auto lastTime = std::chrono::high_resolution_clock::now();
    bool wireframe = false;
    // --- Shadowmap setup ---
    GLuint shadowFBO, shadowMapTex;
    const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMapTex);
    glBindTexture(GL_TEXTURE_2D, shadowMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Direction du soleil et matrices
    glm::vec3 sunDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
    glm::mat4 lightView = glm::lookAt(-sunDir * planetRadius * 10.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-planetRadius*2, planetRadius*2, -planetRadius*2, planetRadius*2, planetRadius*2, planetRadius*20);
    glm::mat4 lightSpaceMatrix = lightProj * lightView;

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
        // Mode switch (TAB = orbite, V = avion)
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            orbitMode = !orbitMode;
            while (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) glfwPollEvents();
        }
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            avionMode = !avionMode;
            while (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) glfwPollEvents();
            if (avionMode) {
                // Init position avion à la position caméra, orientation inchangée
                avionCam.position = camera.position;
                // NE PAS modifier pitch/yaw/roll
            }
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
        if (avionMode) {
            // Contrôle avion (souris = pitch/yaw, Q/E = roll)
            if (leftPressed) {
                avionCam.yaw += dx * 0.2f;
                avionCam.pitch += dy * 0.2f;
                // Suppression de la limitation de pitch pour permettre un tour complet
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) avionCam.roll -= 60.0f * dt;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) avionCam.roll += 60.0f * dt;
            // Calcul direction
            glm::mat4 rot = glm::eulerAngleYXZ(glm::radians(avionCam.yaw), glm::radians(avionCam.pitch), glm::radians(avionCam.roll));
            glm::vec3 forward = glm::vec3(rot * glm::vec4(0,0,-1,0));
            glm::vec3 right = glm::vec3(rot * glm::vec4(1,0,0,0));
            glm::vec3 up = glm::vec3(rot * glm::vec4(0,1,0,0));
            // ZQSD pour avancer/reculer/gauche/droite
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) avionCam.position += forward * avionCam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) avionCam.position -= forward * avionCam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) avionCam.position -= right * avionCam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) avionCam.position += right * avionCam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) avionCam.position += up * avionCam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) avionCam.position -= up * avionCam.speed * dt;
            cameraFront = forward;
            cameraUp = up;
        } else if (orbitMode) {
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
        if (!orbitMode && !avionMode) {
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
        // --- 1. Render shadowmap ---
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        // Utilise un shader simple qui écrit la profondeur depuis le point de vue du soleil
        glUseProgram(shadowProgram); // shadowProgram = shader qui écrit la profondeur
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        icosphere.draw();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- 2. Render scene ---
        glViewport(0, 0, winWidth, winHeight);
        float aspect = (float)winWidth / (float)winHeight;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 10.0f, 200000000.0f);
        glm::mat4 view;
        glm::vec3 camPos;
        if (avionMode) {
            view = glm::lookAt(avionCam.position, avionCam.position + cameraFront, cameraUp);
            camPos = avionCam.position;
        } else if (orbitMode) {
            view = camera.getViewMatrix();
            camPos = camera.position;
        } else {
            view = glm::lookAt(camera.position, camera.position + cameraFront, cameraUp);
            camPos = camera.position;
        }

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); // Désactive le face culling

        // Soleil (sphère lisse émissive)
        glUseProgram(sunProgram);
        glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
        // Pour le soleil, on ignore toute rotation locale, il reste à sa position absolue
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(sunProgram, "sunColor"), 1.0f, 0.95f, 0.8f);
        sun.draw();

        // Planète
        glUseProgram(planetProgram);
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(planetProgram, "planetRadius"), planetRadius);
        glUniform3f(glGetUniformLocation(planetProgram, "planetCenter"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(planetProgram, "sunDirection"), sunDir.x, sunDir.y, sunDir.z);
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glUniform3f(glGetUniformLocation(planetProgram, "cameraPos"), camPos.x, camPos.y, camPos.z);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowMapTex);
        glUniform1i(glGetUniformLocation(planetProgram, "shadowMap"), 0);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        icosphere.draw();

        glfwSwapBuffers(window);
    }
    icosphere.cleanupGLBuffers();
    sun.cleanupGLBuffers();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
