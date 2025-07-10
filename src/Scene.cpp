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
layout (location = 2) in vec3 aNormal;
layout (location = 3) in float aMorph;

out vec3 vertexColor;
out vec3 worldPos;
out vec3 normal;
out float morphFactor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float patchMorph = 0.0;

void main() {
    worldPos = vec3(model * vec4(aPos, 1.0));
    normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
    morphFactor = patchMorph + aMorph; // aMorph=0 par défaut, patchMorph pour le LOD
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec3 worldPos;
in vec3 normal;
in float morphFactor;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 sunPos;
uniform float sunRadius;
uniform bool isSun;
uniform float time;

// Ray marching parameters
const int MAX_STEPS = 100;
const float MIN_DISTANCE = 0.01;
const float MAX_DISTANCE = 50000.0;

// Distance function for sphere (sun)
float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}

// Ray marching for volumetric lighting from sun
float rayMarchLight(vec3 pointPos, vec3 lightDir, float distanceToSun) {
    // Simple attenuation based on distance from sun
    float baseIntensity = 1.0 / (1.0 + 0.00001 * distanceToSun);
    
    // Add some atmospheric scattering effect
    vec3 sunToPoint = pointPos - sunPos;
    float distanceFromSunCenter = length(sunToPoint);
    
    // Light intensity decreases with distance but never goes to zero
    float lightIntensity = max(0.1, baseIntensity);
    
    return lightIntensity;
}

void main() {
    if (isSun) {
        // Sun rendering with glow effect
        vec3 sunColor = vec3(1.0, 0.8, 0.2);
        
        // Add pulsing effect
        float pulse = 0.8 + 0.2 * sin(time * 2.0);
        
        // Add surface detail using noise-like function
        vec3 localPos = normalize(worldPos - sunPos);
        float surface = sin(localPos.x * 20.0) * sin(localPos.y * 20.0) * sin(localPos.z * 20.0);
        surface = surface * 0.1 + 0.9;
        
        FragColor = vec4(sunColor * pulse * surface * 2.0, 1.0);
    } else {
        // Planet rendering with lighting
        vec3 norm = normalize(normal);
        vec3 lightDir = normalize(sunPos - worldPos);
        vec3 viewDir = normalize(viewPos - worldPos);
        
        // Calculate distance to sun for attenuation
        float distanceToSun = length(sunPos - worldPos);
        
        // Use ray marching to calculate light intensity
        float lightIntensity = rayMarchLight(worldPos, lightDir, distanceToSun);
        
        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Specular lighting
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        
        // Ambient lighting
        vec3 ambient = 0.1 * vertexColor;
        
        // Sun light color
        vec3 lightColor = vec3(1.0, 0.9, 0.7);
        
        // Combine lighting
        vec3 diffuse = diff * lightColor * vertexColor * lightIntensity;
        vec3 specular = spec * lightColor * 0.5 * lightIntensity;
        
        vec3 result = ambient + diffuse + specular;
        // --- Geomorphing LOD: fondu entre niveaux (optionnel, ici simple modulation de la couleur) ---
        result = mix(result, vec3(1.0,0.0,1.0), morphFactor*0.1); // Debug: teinte violette si morphing
        FragColor = vec4(result, 1.0);
    }
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
                 m_timeScale(1.0f), m_showTimeControls(true),
                 m_shaderProgram(0), 
                 m_computeProgram(0), m_positionBuffer(0), m_velocityBuffer(0), m_colorBuffer(0),
                 m_time(0.0f) {
    // Créer l'UI
    m_ui = std::make_unique<UI>();
    // Paramètres pour la planète unique dans notre système simplifié
    float planetOrbitRadius = 14960.0f; // Distance Terre-Soleil
    float planetRadius = 0.637f; // Rayon de la Terre
    float altitude = 10.0f; // 10 unités = 100 km d'altitude
    
    // Position de la planète (à 0 degrés, sur l'axe X positif)
    glm::vec3 planetPosition = glm::vec3(planetOrbitRadius, 0.0f, 0.0f);
    
    // Position initiale de la caméra près de la Terre
    // Calculer la position à une certaine altitude au-dessus de la surface de la planète
    glm::vec3 cameraPosition = planetPosition + glm::vec3(0.0f, planetRadius + altitude, 0.0f);
    
    // Créer la caméra avec orientation vers la surface de la Terre (regardant légèrement vers le bas)
    m_camera = std::make_unique<Camera>(cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, -45.0f);
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
    
    // Afficher les informations sur les contrôles de temps (désormais en GUI)
    std::cout << "Contrôles de temps disponibles via l'interface graphique (touche T)" << std::endl;
}

// Effectue le rendu de la scène - Dessine toutes les planètes
void Scene::render() {
    // Clear the screen
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Apply camera collision with all planets
    applyCameraCollision();
    
    // Début du rendu ImGui
    if (m_ui) {
        m_ui->beginFrame();
        
        // Afficher la fenêtre de contrôle du temps si activée
        if (m_showTimeControls) {
            m_ui->showTimeControlsWindow(m_timeScale);
        }
    }
    
    // Use the shader program
    glUseProgram(m_shaderProgram);
    
    // Set global uniforms
    glm::vec3 cameraPos = m_camera->getPosition();
    glUniform3f(glGetUniformLocation(m_shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    
    // Time for sun animation
    m_time += m_deltaTime;
    glUniform1f(glGetUniformLocation(m_shaderProgram, "time"), m_time);
    
    // Sun properties (assuming first planet is the sun)
    if (!m_planets.empty()) {
        glm::vec3 sunPos = m_planets[0]->getPosition();
        float sunRadius = m_planets[0]->getRadius();
        glUniform3f(glGetUniformLocation(m_shaderProgram, "sunPos"), sunPos.x, sunPos.y, sunPos.z);
        glUniform1f(glGetUniformLocation(m_shaderProgram, "sunRadius"), sunRadius);
    }
    
    // Rendu LOD sphérique pour chaque planète
    for (size_t i = 0; i < m_planets.size(); ++i) {
        bool isSun = (i == 0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "isSun"), isSun ? 1 : 0);
        m_planets[i]->updateLOD(m_camera.get());
        m_planets[i]->renderLOD(m_shaderProgram, m_camera.get(), 1440.0f / 900.0f);
    }
    
    // Finir le rendu ImGui après avoir dessiné les planètes
    if (m_ui) {
        m_ui->render();
    }
}

// Nettoie et libère toutes les ressources OpenGL
void Scene::cleanup() {
    // Clean up UI
    if (m_ui) {
        m_ui->cleanup();
    }
    
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

// Met à jour la scène avec le temps delta - Met à jour toutes les planètes et l'orbite
void Scene::update(float deltaTime) {
    // Applique le facteur d'échelle temporel
    float scaledDeltaTime = deltaTime * m_timeScale;
    m_deltaTime = scaledDeltaTime;
    m_lastFrame += deltaTime; // Le lastFrame reste inchangé pour maintenir un temps réel pour l'UI
    
    // Update all planets (rotation sur elles-mêmes)
    for (auto& planet : m_planets) {
        planet->update(scaledDeltaTime); // Utilise le temps mis à l'échelle
    }
    
    // Mouvement orbital de la planète autour du soleil
    if (m_planets.size() > 1) {
        // Vitesse orbitale réaliste à l'échelle de la Terre
        // 2π / (365.25 * 24 * 60 * 60) = 2π / 31557600 = 0.0000002 rad/s
        // Cela donne exactement une année terrestre pour une révolution complète
        static float earthYearInSeconds = 31557600.0f;
        static float orbitalSpeed = 2.0f * M_PI / earthYearInSeconds;
        static float orbitalAngle = 0.0f;
        
        // Calcul du nouvel angle orbital
        orbitalAngle += orbitalSpeed * scaledDeltaTime; // Utilise le temps mis à l'échelle
        
        // Maintien de l'angle entre 0 et 2π
        while (orbitalAngle >= 2.0f * M_PI) {
            orbitalAngle -= 2.0f * M_PI;
        }
        
        // Distance orbitale constante (14960 unités = distance Terre-Soleil)
        float planetOrbitRadius = 14960.0f;
        
        // Inclinaison de l'orbite (environ 23.5 degrés pour la Terre, mais réduite pour visibilité)
        const float orbitalInclination = 23.5f * M_PI / 180.0f * 0.2f; // 20% de l'inclinaison réelle
        
        // Mise à jour de la position de la planète avec inclinaison
        glm::vec3 newPosition = glm::vec3(
            planetOrbitRadius * cos(orbitalAngle),
            planetOrbitRadius * sin(orbitalAngle) * sin(orbitalInclination), // Inclinaison de l'orbite
            planetOrbitRadius * sin(orbitalAngle) * cos(orbitalInclination)
        );
        
        // Mise à jour de la position de la planète
        m_planets[1]->setPosition(newPosition);
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
    // Système solaire simplifié avec une seule planète similaire à la Terre
    // Échelle réelle : 10km = 1 unité
    
    // Soleil au centre - rayon réel: 696 340 km => 69.634 unités
    addPlanet(glm::vec3(0.0f, 0.0f, 0.0f), 69.634f, glm::vec3(1.0f, 0.8f, 0.2f));
    
    // Position initiale de la planète (arbitrairement choisie à 0 degrés)
    float planetAngle = 0.0f;
    
    // Planète similaire à la Terre - rayon réel: 6 371 km => 0.637 unités, distance moyenne: 149.6 millions km => 14960 unités
    float planetOrbitRadius = 14960.0f;
    addPlanet(glm::vec3(
        planetOrbitRadius * cos(planetAngle), 
        0.0f, // Pas d'inclinaison orbitale
        planetOrbitRadius * sin(planetAngle)
    ), 0.637f, glm::vec3(0.2f, 0.6f, 1.0f)); // Couleur bleue comme la Terre
    
    // Aucune rotation pour les planètes (immobiles sur elles-mêmes)
    // Le soleil n'a pas besoin de rotation non plus
    m_planets[0]->setRotationSpeed(0.0f);  // Soleil sans rotation
    m_planets[1]->setRotationSpeed(0.0f);  // Planète sans rotation
    
    std::cout << "Created simplified solar system with " << m_planets.size() << " celestial bodies at real scale (10km = 1 unit)" << std::endl;
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
    
    // Contrôles du temps de simulation
    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        m_showTimeControls = !m_showTimeControls;
        if (m_ui) {
            m_ui->setShowTimeControls(m_showTimeControls);
        }
        
        tKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tKeyPressed = false;
    }
    
    // Augmenter la vitesse de simulation (touche +)
    static bool plusKeyPressed = false;
    if ((glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || 
         glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) && !plusKeyPressed) {
        m_timeScale *= 2.0f;
        // L'affichage de la vitesse se fait maintenant dans l'UI
        plusKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_RELEASE && 
               glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_RELEASE) {
        plusKeyPressed = false;
    }
    
    // Diminuer la vitesse de simulation (touche -)
    static bool minusKeyPressed = false;
    if ((glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || 
         glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) && !minusKeyPressed) {
        m_timeScale *= 0.5f;
        // L'affichage de la vitesse se fait maintenant dans l'UI
        minusKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE && 
               glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_RELEASE) {
        minusKeyPressed = false;
    }
    
    // Réinitialiser la vitesse de simulation (touche 0)
    static bool zeroKeyPressed = false;
    if ((glfwGetKey(window, GLFW_KEY_KP_0) == GLFW_PRESS || 
         glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) && !zeroKeyPressed) {
        m_timeScale = 1.0f;
        // L'affichage de la vitesse se fait maintenant dans l'UI
        zeroKeyPressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_KP_0) == GLFW_RELEASE && 
               glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE) {
        zeroKeyPressed = false;
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
        // Limite l'approche à 0.005 unité de la surface (environ 50 km à l'échelle réelle)
        float minimumDistance = 0.005f; // Distance minimale à la surface
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

// Initialise l'interface utilisateur avec une fenêtre GLFW
void Scene::initializeUI(GLFWwindow* window) {
    if (m_ui) {
        m_ui->initialize(window);
        m_ui->setShowTimeControls(m_showTimeControls);
    }
}