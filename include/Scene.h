#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include "Camera.h"
#include "Planet.h"

class Scene {
private:
    glm::vec3 m_backgroundColor;        // Couleur de fond de la scène
    float m_lastFrame;                  // Temps du dernier frame
    float m_deltaTime;                  // Temps écoulé depuis le dernier frame
    
    // Objets OpenGL
    unsigned int m_shaderProgram;       // Programme shader pour le rendu
    
    // Objets compute shader
    unsigned int m_computeProgram;      // Programme de compute shader
    unsigned int m_positionBuffer;      // Buffer des positions des particules
    unsigned int m_velocityBuffer;      // Buffer des vitesses des particules
    unsigned int m_colorBuffer;         // Buffer des couleurs des particules
    
    // Paramètres de simulation GPU
    static const int PARTICLE_COUNT = 1000000; // 1 million de particules
    float m_time;                       // Temps global
    
    // Planètes
    std::vector<std::unique_ptr<Planet>> m_planets;  // Collection des planètes
    
    // Caméra
    std::unique_ptr<Camera> m_camera;   // Caméra de la scène
    
    // Méthodes privées
    void setupShaders();                        // Configure les shaders de rendu
    void setupComputeShaders();                 // Configure les shaders de calcul
    void setupGPUBuffers();                     // Configure les buffers GPU
    void setupPlanets();                        // Configure les planètes par défaut
    void applyCameraCollision();                // Gère les collisions caméra-planètes
    unsigned int compileShader(unsigned int type, const char* source);  // Compile un shader
    unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);  // Crée un programme shader
    unsigned int createComputeProgram(const char* computeSource);  // Crée un programme de compute shader

public:
    Scene();                            // Constructeur
    ~Scene();                           // Destructeur

    void initialize();                  // Initialise la scène
    void render();                      // Effectue le rendu
    void cleanup();                     // Nettoie les ressources
    void update(float deltaTime);       // Met à jour la scène
    
    void setBackgroundColor(float r, float g, float b);  // Définit la couleur de fond
    void processInput(GLFWwindow* window);               // Gère les entrées clavier
    void processMouseMovement(float xoffset, float yoffset);  // Gère le mouvement de la souris
    void setWireframeMode(bool enabled);                 // Active/désactive le mode wireframe
    Camera* getCamera() { return m_camera.get(); }      // Retourne la caméra
    
    // Gestion des planètes
    void addPlanet(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));  // Ajoute une planète
    void addPlanet(std::unique_ptr<Planet> planet);     // Ajoute une planète existante
    Planet* getPlanet(size_t index);                    // Récupère une planète par index
    size_t getPlanetCount() const { return m_planets.size(); }  // Retourne le nombre de planètes
    void clearPlanets();                                // Supprime toutes les planètes
    const std::vector<std::unique_ptr<Planet>>& getPlanets() const { return m_planets; }  // Accès aux planètes pour debug
};
