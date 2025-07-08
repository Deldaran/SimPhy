#include "Scene.h"
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <functional>
#include <unordered_map>
#include <limits>
#include <algorithm>

// Force l'utilisation du GPU dédié (RTX 4060) au lieu du GPU intégré
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

// Compute shader source for GPU particle simulation
const char* computeShaderSource = R"(
#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer PositionBuffer {
    vec4 positions[];
};

layout(std430, binding = 1) buffer VelocityBuffer {
    vec4 velocities[];
};

layout(std430, binding = 2) buffer ColorBuffer {
    vec4 colors[];
};

uniform float time;
uniform float deltaTime;

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index >= positions.length()) return;
    
    vec3 pos = positions[index].xyz;
    vec3 vel = velocities[index].xyz;
    
    // Simple gravity simulation
    vec3 gravity = vec3(0.0, -9.81, 0.0);
    vel += gravity * deltaTime;
    
    // Update position
    pos += vel * deltaTime;
    
    // Bounce off ground
    if (pos.y < -5.0) {
        pos.y = -5.0;
        vel.y = -vel.y * 0.8; // Energy loss
    }
    
    // Update buffers
    positions[index] = vec4(pos, 1.0);
    velocities[index] = vec4(vel, 0.0);
    
    // Color based on velocity
    float speed = length(vel);
    colors[index] = vec4(speed * 0.1, 1.0 - speed * 0.1, 0.5, 1.0);
}
)";

// Vertex shader for particles
const char* particleVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;

out vec3 vertexColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * aPos;
    gl_PointSize = 2.0;
    vertexColor = aColor.rgb;
}
)";

// Fragment shader for particles
const char* particleFragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

// Constructeur de la scène - Initialise tous les membres et crée la caméra
Scene::Scene() : m_backgroundColor(0.05f, 0.05f, 0.15f), m_lastFrame(0.0f), m_deltaTime(0.0f), 
                 m_shaderProgram(0), 
                 m_computeProgram(0), m_positionBuffer(0), m_velocityBuffer(0), m_colorBuffer(0),
                 m_time(0.0f) {
    // Créer la caméra - Position initiale près de la Terre pour la voir (nouvelle taille)
    m_camera = std::make_unique<Camera>(glm::vec3(5500.0f, 0.0f, 0.0f));
}

// Destructeur de la scène - Appelle la méthode cleanup pour libérer les ressources
Scene::~Scene() {
    cleanup();
}

// Initialise la scène - Configure OpenGL, les shaders et les planètes
void Scene::initialize() {
    // Vérifier et afficher le GPU utilisé
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    
    std::cout << "=== GPU INFORMATION ===" << std::endl;
    std::cout << "Vendor: " << vendor << std::endl;
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "=======================" << std::endl;
    
    // Vérifier si on utilise le GPU dédié
    if (strstr(renderer, "RTX") || strstr(renderer, "GTX") || strstr(renderer, "NVIDIA")) {
        std::cout << "✓ GPU dédié NVIDIA détecté!" << std::endl;
    } else if (strstr(renderer, "Intel") || strstr(renderer, "HD Graphics")) {
        std::cout << "⚠ GPU intégré Intel détecté! Vérifiez les paramètres." << std::endl;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable point size for particles
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    // Setup shaders
    setupShaders();
    
    // Setup compute shaders
    setupComputeShaders();
    
    // Setup GPU buffers
    setupGPUBuffers();
    
    // Setup planets
    setupPlanets();
    
    std::cout << "Scene initialized with GPU acceleration" << std::endl;
}

// Effectue le rendu de la scène - Dessine toutes les planètes
void Scene::render() {
    // Clear the screen
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Apply camera collision with all planets
    applyCameraCollision();
    
    // Render all planets
    for (auto& planet : m_planets) {
        planet->render(m_shaderProgram, m_camera.get(), 1440.0f / 900.0f);
    }
}

// Nettoie et libère toutes les ressources OpenGL
void Scene::cleanup() {
    // Clean up planets
    for (auto& planet : m_planets) {
        planet->cleanup();
    }
    m_planets.clear();
    
    // Clean up OpenGL objects
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    
    // Clean up compute shader objects
    if (m_computeProgram) {
        glDeleteProgram(m_computeProgram);
        m_computeProgram = 0;
    }
    if (m_positionBuffer) {
        glDeleteBuffers(1, &m_positionBuffer);
        m_positionBuffer = 0;
    }
    if (m_velocityBuffer) {
        glDeleteBuffers(1, &m_velocityBuffer);
        m_velocityBuffer = 0;
    }
    if (m_colorBuffer) {
        glDeleteBuffers(1, &m_colorBuffer);
        m_colorBuffer = 0;
    }
}

// Définit la couleur de fond de la scène
void Scene::setBackgroundColor(float r, float g, float b) {
    m_backgroundColor = glm::vec3(r, g, b);
}

// Met à jour la scène avec le temps delta - Met à jour toutes les planètes
void Scene::update(float deltaTime) {
    m_deltaTime = deltaTime;
    m_lastFrame += deltaTime;
    
    // Update all planets
    for (auto& planet : m_planets) {
        planet->update(deltaTime);
    }
}

// Configure les shaders de rendu pour les planètes
void Scene::setupShaders() {
    m_shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
}

// Configure les shaders de calcul pour la simulation GPU
void Scene::setupComputeShaders() {
    m_computeProgram = createComputeProgram(computeShaderSource);
}

// Configure les buffers GPU pour la simulation de particules
void Scene::setupGPUBuffers() {
    // Initialize particle data on CPU
    std::vector<glm::vec4> positions(PARTICLE_COUNT);
    std::vector<glm::vec4> velocities(PARTICLE_COUNT);
    std::vector<glm::vec4> colors(PARTICLE_COUNT);
    
    // Initialize with random positions and velocities
    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        positions[i] = glm::vec4(
            (rand() % 1000 - 500) / 100.0f,  // x: -5 to 5
            (rand() % 1000) / 100.0f,        // y: 0 to 10
            (rand() % 1000 - 500) / 100.0f,  // z: -5 to 5
            1.0f
        );
        velocities[i] = glm::vec4(
            (rand() % 200 - 100) / 100.0f,   // vx: -1 to 1
            0.0f,                            // vy: 0
            (rand() % 200 - 100) / 100.0f,   // vz: -1 to 1
            0.0f
        );
        colors[i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Create GPU buffers
    glGenBuffers(1, &m_positionBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, positions.size() * sizeof(glm::vec4), positions.data(), GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &m_velocityBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, velocities.size() * sizeof(glm::vec4), velocities.data(), GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &m_colorBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_colorBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_DYNAMIC_DRAW);
    
    // Bind buffers to binding points
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_velocityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_colorBuffer);
}

// Configure les planètes par défaut dans la scène
void Scene::setupPlanets() {
    // Système solaire à l'échelle avec des tailles augmentées pour une meilleure visibilité
    
    // Soleil au centre (jaune-orange, énorme)
    addPlanet(glm::vec3(0.0f, 0.0f, 0.0f), 1000.0f, glm::vec3(1.0f, 0.8f, 0.2f));
    
    // Mercure (gris, petit, proche du soleil)
    addPlanet(glm::vec3(2000.0f, 0.0f, 0.0f), 100.0f, glm::vec3(0.7f, 0.7f, 0.7f));
    
    // Vénus (orange-jaune, moyen)
    addPlanet(glm::vec3(3500.0f, 0.0f, 0.0f), 200.0f, glm::vec3(1.0f, 0.7f, 0.3f));
    
    // Terre (bleu-vert, moyen)
    addPlanet(glm::vec3(5000.0f, 0.0f, 0.0f), 250.0f, glm::vec3(0.2f, 0.6f, 1.0f));
    
    // Mars (rouge, moyen)
    addPlanet(glm::vec3(7000.0f, 0.0f, 0.0f), 150.0f, glm::vec3(0.8f, 0.3f, 0.2f));
    
    // Jupiter (orange-brun, très grand)
    addPlanet(glm::vec3(10000.0f, 0.0f, 0.0f), 500.0f, glm::vec3(0.9f, 0.6f, 0.3f));
    
    // Saturne (jaune-brun, grand)
    addPlanet(glm::vec3(15000.0f, 0.0f, 0.0f), 400.0f, glm::vec3(0.9f, 0.8f, 0.5f));
    
    // Uranus (bleu-cyan, moyen)
    addPlanet(glm::vec3(25000.0f, 0.0f, 0.0f), 300.0f, glm::vec3(0.3f, 0.8f, 0.9f));
    
    // Neptune (bleu foncé, moyen)
    addPlanet(glm::vec3(35000.0f, 0.0f, 0.0f), 280.0f, glm::vec3(0.2f, 0.4f, 0.9f));
    
    // Définir les rotations des planètes (vitesses ultra réduites pour un rendu très réaliste)
    if (m_planets.size() >= 9) {
        m_planets[0]->setRotationSpeed(0.001f);  // Soleil - rotation extrêmement lente
        m_planets[1]->setRotationSpeed(0.005f);  // Mercure - rotation très lente
        m_planets[2]->setRotationSpeed(-0.009f); // Vénus - rotation rétrograde ultra lente
        m_planets[3]->setRotationSpeed(0.008f);  // Terre - rotation très lente
        m_planets[3]->setRotationAxis(glm::vec3(0.1f, 1.0f, 0.0f)); // Terre légèrement inclinée
        m_planets[4]->setRotationSpeed(0.007f);  // Mars - rotation très lente
        m_planets[5]->setRotationSpeed(0.008f);  // Jupiter - rotation lente
        m_planets[6]->setRotationSpeed(0.001f);  // Saturne - rotation lente
        m_planets[6]->setRotationAxis(glm::vec3(0.5f, 1.0f, 0.1f)); // Saturne inclinée
        m_planets[7]->setRotationSpeed(-0.001f); // Uranus - rotation rétrograde très lente
        m_planets[7]->setRotationAxis(glm::vec3(0.9f, 0.4f, 0.0f)); // Uranus très inclinée
        m_planets[8]->setRotationSpeed(0.008f);  // Neptune - rotation très lente
        
        // Planètes supplémentaires
        if (m_planets.size() >= 11) {
            m_planets[9]->setRotationSpeed(0.006f);
            m_planets[9]->setRotationAxis(glm::vec3(0.3f, 1.0f, 0.7f));
            m_planets[10]->setRotationSpeed(-0.004f);
            m_planets[10]->setRotationAxis(glm::vec3(0.8f, 0.2f, 1.0f));
        }
    }
    
    std::cout << "Created solar system with " << m_planets.size() << " celestial bodies" << std::endl;
}

// Compile un shader OpenGL avec vérification d'erreur
unsigned int Scene::compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

// Crée un programme shader en liant vertex et fragment shaders
unsigned int Scene::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Crée un programme de compute shader pour les calculs GPU
unsigned int Scene::createComputeProgram(const char* computeSource) {
    unsigned int computeShader = compileShader(GL_COMPUTE_SHADER, computeSource);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);
    
    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::COMPUTE::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(computeShader);
    return program;
}

// Gère les entrées clavier pour le déplacement de la caméra
void Scene::processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::FORWARD, m_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::BACKWARD, m_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::LEFT, m_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::RIGHT, m_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::UP, m_deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        m_camera->processKeyboard(Camera::DOWN, m_deltaTime);
        
    // Toggle wireframe mode
    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
        static bool wireframe = false;
        wireframe = !wireframe;
        setWireframeMode(wireframe);
        fKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fKeyPressed = false;
    }
}

// Gère le mouvement de la souris pour la rotation de la caméra
void Scene::processMouseMovement(float xoffset, float yoffset) {
    // Les offsets sont déjà calculés dans Window.cpp
    // Pas besoin de recalculer avec m_lastX et m_lastY
    
    // Appliquer directement les offsets à la caméra
    m_camera->processMouseMovement(xoffset, yoffset);
}

// Active ou désactive le mode wireframe (filaire)
void Scene::setWireframeMode(bool enabled) {
    if (enabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// Gère les collisions de la caméra avec toutes les planètes (adapté aux grandes échelles)
void Scene::applyCameraCollision() {
    glm::vec3 cameraPos = m_camera->getPosition();
    
    // Trouver la distance à l'objet le plus proche pour ajuster la vitesse
    float nearestDistance = std::numeric_limits<float>::max();
    
    // Check collision with all planets
    float minSpeedMultiplier = 1.0f;
    glm::vec3 correctedPosition = cameraPos;
    
    for (const auto& planet : m_planets) {
        // Calculate distance to planet surface
        float distanceToSurface = planet->getDistanceToSurface(cameraPos);
        nearestDistance = std::min(nearestDistance, std::max(0.1f, distanceToSurface));
        
        // Get speed multiplier for this planet
        float speedMultiplier = planet->getSpeedMultiplierNearSurface(cameraPos);
        minSpeedMultiplier = std::min(minSpeedMultiplier, speedMultiplier);
        
        // Check if camera needs to be pushed away from this planet
        // Limite l'approche à 0.11 unité de la surface (pas plus proche)
        float minimumDistance = 0.11f; // Distance minimale à la surface
        glm::vec3 newPosition = planet->preventPenetration(correctedPosition, minimumDistance);
        if (glm::length(newPosition - correctedPosition) > 0.0001f) {
            correctedPosition = newPosition;
        }
    }
    
    // Ajuster la vitesse de base selon la distance aux objets
    m_camera->adjustSpeedBasedOnDistance(nearestDistance);
    
    // Apply the most restrictive speed multiplier
    m_camera->setSpeedMultiplier(minSpeedMultiplier);
    
    // Apply corrected position if needed
    if (glm::length(correctedPosition - cameraPos) > 0.01f) {
        m_camera->setPosition(correctedPosition);
    }
}

// Planet management methods
// Ajoute une planète à la scène avec position, rayon et couleur
void Scene::addPlanet(const glm::vec3& position, float radius, const glm::vec3& color) {
    auto planet = std::make_unique<Planet>(position, radius, color);
    planet->initialize();
    m_planets.push_back(std::move(planet));
}

// Ajoute une planète existante à la scène
void Scene::addPlanet(std::unique_ptr<Planet> planet) {
    if (planet) {
        planet->initialize();
        m_planets.push_back(std::move(planet));
    }
}

// Récupère un pointeur vers une planète par son index
Planet* Scene::getPlanet(size_t index) {
    if (index < m_planets.size()) {
        return m_planets[index].get();
    }
    return nullptr;
}

// Supprime toutes les planètes de la scène
void Scene::clearPlanets() {
    for (auto& planet : m_planets) {
        planet->cleanup();
    }
    m_planets.clear();
}