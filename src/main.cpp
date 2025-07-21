#include <vector>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Icosphere.h"
#include "Camera.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>
#include "tiny_obj_loader.h"

struct SceneObject {
    GLuint vao = 0, vbo = 0, ebo = 0;
    size_t indexCount = 0;
    glm::mat4 model = glm::mat4(1.0f);
    int shaderProgram = 0;
    std::string objPath;
};

std::vector<SceneObject> sceneObjects;

// Charge un .obj et ajoute un objet à la scène
void addObjectToScene(const std::string& objPath, const glm::mat4& model, int shaderProgram) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str());
    if (!ret) {
        printf("Erreur chargement OBJ: %s\n", err.c_str());
        return;
    }
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    // 1. Calcul du barycentre du mesh
    glm::vec3 sum(0.0f);
    int vertCount = 0;
    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        sum.x += attrib.vertices[i+0];
        sum.y += attrib.vertices[i+1];
        sum.z += attrib.vertices[i+2];
        vertCount++;
    }
    glm::vec3 barycenter = vertCount > 0 ? sum / float(vertCount) : glm::vec3(0.0f);

    // 2. Ajout des sommets centrés
    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                float vx = attrib.vertices[3 * idx.vertex_index + 0] - barycenter.x;
                float vy = attrib.vertices[3 * idx.vertex_index + 1] - barycenter.y;
                float vz = attrib.vertices[3 * idx.vertex_index + 2] - barycenter.z;
                vertices.push_back(vx);
                vertices.push_back(vy);
                vertices.push_back(vz);
                indices.push_back((unsigned int)indices.size());
            }
            index_offset += fv;
        }
    }
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    SceneObject obj;
    obj.vao = vao;
    obj.vbo = vbo;
    obj.ebo = ebo;
    obj.indexCount = indices.size();
    obj.model = model;
    obj.shaderProgram = shaderProgram;
    obj.objPath = objPath;
    sceneObjects.push_back(obj);
}

// Fonction utilitaire globale pour lire un fichier texte (shader)
std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        printf("Erreur ouverture fichier shader: %s\n", path.c_str());
    } else {
        printf("Ouverture OK fichier shader: %s\n", path.c_str());
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

const float planetRadius = 6371.0f * 300.0f; // planète 300x plus grande
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

int main() {
    printf("[LOG] Début main()\n");

    printf("[LOG] Avant glfwInit()\n");
    if (!glfwInit()) { printf("[LOG] Erreur glfwInit()\n"); return -1; }
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    int winWidth = 1280;
    int winHeight = 720;
    printf("[LOG] Avant glfwCreateWindow()\n");
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Icosphere Planet", nullptr, nullptr);
    if (!window) { printf("[LOG] Erreur glfwCreateWindow()\n"); glfwTerminate(); return -1; }
    printf("[LOG] Avant glfwMakeContextCurrent()\n");
    glfwMakeContextCurrent(window);
    printf("[LOG] Avant gladLoadGLLoader()\n");
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    printf("[LOG] OpenGL context et GLAD initialisés\n");

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


    // Inclinaison de l'axe de la planète (obliquité ~23.5°)
    float obliquity = glm::radians(23.5f);
    glm::mat4 planetTilt = glm::rotate(glm::mat4(1.0f), obliquity, glm::vec3(1,0,0));

    // Création de l'icosphere et des buffers
    Icosphere icosphere(glm::vec3(0.0f), planetRadius, 5, true);
    icosphere.initGLBuffers();

    // Direction du soleil sur l'équateur (axe X global), puis inclinaison obliquité
    glm::vec3 baseSunDir = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
    sunDir = glm::vec3(glm::rotate(glm::mat4(1.0f), obliquity, glm::vec3(1,0,0)) * glm::vec4(baseSunDir, 0.0f));


    // Chargement de la heightmap générée
    int texW, texH, texC;
    unsigned char* heightData = stbi_load("heightmap.png", &texW, &texH, &texC, 1);
    GLuint heightmapTex = 0;
    if (heightData) {
        glGenTextures(1, &heightmapTex);
        glBindTexture(GL_TEXTURE_2D, heightmapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texW, texH, 0, GL_RED, GL_UNSIGNED_BYTE, heightData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        stbi_image_free(heightData);
    }

    // Création du soleil (utilise la même classe que la planète, mais sans relief)
    glm::vec3 sunPos = glm::vec3(0.0f) + sunDir * planetRadius * 100.0f; // distance 100x le rayon planète
    Icosphere sunIcosphere(sunPos, planetRadius * 10.0f, 5, false); // soleil 10x plus gros
    sunIcosphere.initGLBuffers();

    // --- Shaders du vaisseau (après contexte OpenGL) ---
    int shipVert = compileShader(readFile("ship.vert"), GL_VERTEX_SHADER);
    int shipFrag = compileShader(readFile("ship.frag"), GL_FRAGMENT_SHADER);
    int shipProgram = glCreateProgram();
    glAttachShader(shipProgram, shipVert);
    glAttachShader(shipProgram, shipFrag);
    glLinkProgram(shipProgram);
    glDeleteShader(shipVert);
    glDeleteShader(shipFrag);
    printf("[LOG] Shaders vaisseau compilés et liés\n");
    // Shaders planète
    printf("[LOG] Avant compilation shaders planète\n");
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
    printf("[LOG] Shaders planète compilés et liés\n");
    // Shaders soleil
    printf("[LOG] Avant compilation shaders soleil\n");
    int sunVert = compileShader(readFile("sun.vert"), GL_VERTEX_SHADER);
    int sunFrag = compileShader(readFile("sun.frag"), GL_FRAGMENT_SHADER);
    int sunProgram = glCreateProgram();
    glAttachShader(sunProgram, sunVert);
    glAttachShader(sunProgram, sunFrag);
    glLinkProgram(sunProgram);
    glDeleteShader(sunVert);
    glDeleteShader(sunFrag);
    printf("[LOG] Shaders soleil compilés et liés\n");

    

    // const float planetRadius = 6371.0f; // SUPPRIMÉ : on garde la version géante définie en haut
    // Inclinaison initiale de la caméra pour orbiter sur l'équateur incliné
    float initialYaw = 0.0f;
    float initialPitch = glm::degrees(obliquity); // même inclinaison que la planète
    glm::vec3 planetCenter(0.0f);
    // Place le vaisseau à 100km au-dessus de la surface réelle
    glm::vec3 shipCenter = planetCenter + glm::vec3(0.0f, planetRadius * 1.11f, 0.0f);
    bool orbitOnShip = false;
    
    // Ajoute le vaisseau à la scène (exemple)
    addObjectToScene("appolo.obj", glm::translate(glm::mat4(1.0f), shipCenter), shipProgram);
    // Pour ajouter d'autres objets :
    // addObjectToScene("autre.obj", glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)), shaderId);

    // Caméra en orbite autour du centre courant (planète ou vaisseau)
    Camera camera(planetRadius * 1.0f, initialYaw, initialPitch, planetCenter);
    float cameraSpeed = 100.0f;
    double lastX = 0, lastY = 0;
    bool firstMouse = true;
    glm::mat4 model = glm::mat4(1.0f);
    auto lastTime = std::chrono::high_resolution_clock::now();
    bool wireframe = false;

    // Direction du soleil et matrices (déjà inclinée)
    glm::mat4 lightView = glm::lookAt(-sunDir * planetRadius * 10.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-planetRadius*2, planetRadius*2, -planetRadius*2, planetRadius*2, planetRadius*2, planetRadius*20);
    glm::mat4 lightSpaceMatrix = lightProj * lightView;

    printf("[LOG] Début boucle principale\n");
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // --- LOG boucle ---
        //printf("[LOG] Frame\n");
        glfwPollEvents();
        // Wireframe toggle
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            printf("[LOG] Touche F pressée (wireframe toggle)\n");
            wireframe = !wireframe;
            while (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) glfwPollEvents();
        }

        // Touche V : switch centre d'orbite planète <-> vaisseau
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            printf("[LOG] Touche V pressée (switch orbite)\n");
            orbitOnShip = !orbitOnShip;
            if (orbitOnShip) {
                // Place la caméra à une position standard autour du vaisseau
                float shipOrbitRadius = 1000.0f; // 1km autour du vaisseau
                float shipYaw = 0.0f;
                float shipPitch = 20.0f;
                camera = Camera(shipOrbitRadius, shipYaw, shipPitch, shipCenter);
            } else {
                // Place la caméra à une position standard autour de la planète
                float planetOrbitRadius = planetRadius * 3.0f;
                float planetYaw = 0.0f;
                float planetPitch = glm::degrees(glm::radians(23.5f));
                camera = Camera(planetOrbitRadius, planetYaw, planetPitch, planetCenter);
            }
            firstMouse = true;
            while (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) glfwPollEvents();
        }

        // Mouse movement (rotation seulement si clic gauche)
        double xpos_d, ypos_d;
        glfwGetCursorPos(window, &xpos_d, &ypos_d);
        float xpos = static_cast<float>(xpos_d);
        float ypos = static_cast<float>(ypos_d);
        int leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float dx = 0.0f, dy = 0.0f;

        // Calcul dynamique de la vitesse selon la distance au sol (centre planète)
        // Calcul dynamique de la vitesse selon la distance au centre d'orbite courant
        glm::vec3 currentCenter = orbitOnShip ? shipCenter : planetCenter;
        float distToCenter = glm::length(camera.getPosition() - currentCenter);
        float surfaceDist = std::max(distToCenter - planetRadius, 1.0f); // évite division par zéro
        cameraSpeed = 0.05f * surfaceDist; // 0.05f à ajuster selon le ressenti

        // Gestion de la caméra orbite uniquement
        if (leftPressed) {
            printf("[LOG] Clic gauche détecté (rotation caméra)\n");
            if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
            dx = (xpos - static_cast<float>(lastX)) * 0.08f;
            dy = (static_cast<float>(lastY) - ypos) * 0.08f;
            camera.processMouseMovement(dx, dy);
        }
        // Mise à jour à chaque frame
        lastX = xpos;
        lastY = ypos;
        // Zoom/dézoom avec la molette
        if (scrollData.yoffset != 0.0f) {
            // Vitesse de zoom adaptée : planète = normal, vaisseau = lent
            float zoomSpeed = orbitOnShip ? 1.0f : 1000.0f;
            camera.processMouseScroll(scrollData.yoffset * zoomSpeed);
            scrollData.yoffset = 0.0f;
        }

        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);

        // --- 2. Render scene ---
        glViewport(0, 0, winWidth, winHeight);
        float aspect = (float)winWidth / (float)winHeight;
        float nearPlane = orbitOnShip ? 10.0f : 1000.0f;
        float farPlane  = orbitOnShip ? 1e11f : 1e11f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, nearPlane, farPlane);
        glm::mat4 view = camera.getViewMatrix();
        glm::vec3 camPos = camera.getPosition();

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Soleil (toujours sans culling, sans depth test) ---
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUseProgram(sunProgram);
        glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos);
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sunProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(sunProgram, "sunColor"), 1.0f, 0.95f, 0.2f);
        sunIcosphere.draw(true);

        // --- Planète (culling et depth test activés) ---
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glUseProgram(planetProgram);
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "model"), 1, GL_FALSE, glm::value_ptr(planetTilt));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(planetProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(planetProgram, "planetRadius"), planetRadius);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightmapTex);
        glUniform1i(glGetUniformLocation(planetProgram, "heightmapTex"), 1);
        glUniform3f(glGetUniformLocation(planetProgram, "planetCenter"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(planetProgram, "sunDirection"), -sunDir.x, -sunDir.y, -sunDir.z);
        glUniform3f(glGetUniformLocation(planetProgram, "cameraPos"), camPos.x, camPos.y, camPos.z);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        icosphere.draw();

        // --- Objets dynamiques ---
        for (const auto& obj : sceneObjects) {
            // Culling spécifique par objet (exemple : vaisseau sans culling)
            if (obj.objPath == "vaisseau.obj") {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
            glUseProgram(obj.shaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(obj.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(obj.model));
            glUniformMatrix4fv(glGetUniformLocation(obj.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(obj.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(glGetUniformLocation(obj.shaderProgram, "sunDirection"), sunDir.x, sunDir.y, sunDir.z);
            glUniform3f(glGetUniformLocation(obj.shaderProgram, "sunColor"), 0.9f, 0.9f, 0.9f);
            glUniform3f(glGetUniformLocation(obj.shaderProgram, "cameraPos"), camPos.x, camPos.y, camPos.z);
            glBindVertexArray(obj.vao);
            glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }
        glEnable(GL_CULL_FACE); // Réactive pour la suite
        glfwSwapBuffers(window);
    }

    // Nettoyage de tous les objets dynamiques
    for (const auto& obj : sceneObjects) {
        glDeleteVertexArrays(1, &obj.vao);
        glDeleteBuffers(1, &obj.vbo);
        glDeleteBuffers(1, &obj.ebo);
        // glDeleteProgram(obj.shaderProgram); // à activer si chaque objet a son propre shader
    }
    icosphere.cleanupGLBuffers();
    sunIcosphere.cleanupGLBuffers();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
