#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cmath>
#include "Camera.h"

class Planet {
private:
    // Propriétés de la planète
    glm::vec3 m_position;               // Position dans l'espace
    float m_radius;                     // Rayon de la planète
    glm::vec3 m_color;                  // Couleur de base
    glm::vec3 m_rotationAxis;           // Axe de rotation
    float m_rotationSpeed;              // Vitesse de rotation
    float m_currentRotation;            // Angle de rotation actuel
    
    // Objets OpenGL
    unsigned int m_VAO;                 // Vertex Array Object
    unsigned int m_VBO;                 // Vertex Buffer Object
    unsigned int m_EBO;                 // Element Buffer Object
    
    // Données géométriques
    std::vector<glm::vec3> m_vertices;  // Vertices de base
    std::vector<float> m_finalVertices; // Vertices finaux avec couleurs
    std::vector<unsigned int> m_finalIndices; // Indices des triangles
    std::vector<unsigned int> m_culledIndices; // Indices après culling
    int m_sphereVertexCount;            // Nombre de vertices
    int m_sphereIndexCount;             // Nombre d'indices
    int m_culledIndexCount;             // Nombre d'indices après culling
    
    // Système LOD dynamique
    struct Triangle {
        unsigned int v1, v2, v3;        // Indices des vertices
        glm::vec3 center;               // Centre du triangle
        float distanceToCamera;         // Distance à la caméra
        int subdivisionLevel;           // Niveau de subdivision
        bool needsUpdate;               // Nécessite une mise à jour
        bool isVisible;                 // Visible après culling
        glm::vec3 normal;               // Normale du triangle (pour backface culling)
    };
    
    std::vector<Triangle> m_triangles;  // Triangles du maillage
    glm::vec3 m_lastCameraPosition;     // Dernière position de la caméra
    bool m_lodNeedsUpdate;              // Le LOD nécessite une mise à jour
    float m_lastLODUpdateTime;          // Temps de la dernière mise à jour LOD
    
    // Méthodes privées
    void generateSphere();                      // Génère la géométrie sphérique
    void updateDynamicLOD(Camera* camera);      // Met à jour le système LOD
    void rebuildMesh();                         // Reconstruit le maillage
    void performCulling(Camera* camera, float aspectRatio);        // Effectue le culling des triangles
    bool isTriangleInFrustum(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, Camera* camera, float aspectRatio); // Test frustum
    bool isBackfacing(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& cameraPos); // Test backface
    void subdivideTriangleRecursive(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, 
                                   int level, int subdivisionLevel, std::function<int(const glm::vec3&, int)> addVertex); // Subdivision récursive
    glm::vec3 getTriangleCenter(const Triangle& triangle);  // Centre d'un triangle
    glm::vec3 calculateTriangleNormal(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3); // Calcule la normale
    float getDistanceToCamera(const glm::vec3& point, Camera* camera);  // Distance à la caméra
    
public:
    Planet(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f)); // Constructeur
    ~Planet();                                          // Destructeur
    
    void initialize();                                  // Initialise la planète
    void render(unsigned int shaderProgram, Camera* camera, float aspectRatio); // Effectue le rendu
    void update(float deltaTime);                       // Met à jour la planète
    void cleanup();                                     // Nettoie les ressources
    
    // Setters
    void setPosition(const glm::vec3& position) { m_position = position; }      // Définit la position
    void setRadius(float radius);                       // Définit le rayon
    void setColor(const glm::vec3& color) { m_color = color; }                  // Définit la couleur
    void setRotationAxis(const glm::vec3& axis) { m_rotationAxis = glm::normalize(axis); }  // Définit l'axe de rotation
    void setRotationSpeed(float speed) { m_rotationSpeed = speed; }             // Définit la vitesse de rotation
    
    // Getters
    const glm::vec3& getPosition() const { return m_position; }                 // Retourne la position
    float getRadius() const { return m_radius; }                               // Retourne le rayon
    const glm::vec3& getColor() const { return m_color; }                      // Retourne la couleur
    const glm::vec3& getRotationAxis() const { return m_rotationAxis; }        // Retourne l'axe de rotation
    float getRotationSpeed() const { return m_rotationSpeed; }                 // Retourne la vitesse de rotation
    
    // Détection de collision
    bool isPointInside(const glm::vec3& point) const;                          // Vérifie si un point est dans la planète
    float getDistanceToSurface(const glm::vec3& point) const;                  // Distance à la surface
    glm::vec3 preventPenetration(const glm::vec3& point, float margin = 0.3f) const; // Empêche la pénétration
    float getSpeedMultiplierNearSurface(const glm::vec3& point) const;         // Multiplicateur de vitesse près de la surface
    
    // Matrice de transformation
    glm::mat4 getModelMatrix() const;                   // Retourne la matrice de transformation
};

// Fonction de hachage pour glm::vec3 pour utilisation avec unordered_map
struct Vec3Hash {
    std::size_t operator()(const glm::vec3& v) const {
        std::size_t h1 = std::hash<float>{}(v.x);
        std::size_t h2 = std::hash<float>{}(v.y);
        std::size_t h3 = std::hash<float>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Fonction d'égalité pour glm::vec3
struct Vec3Equal {
    bool operator()(const glm::vec3& lhs, const glm::vec3& rhs) const {
        const float epsilon = 1e-6f;
        return std::abs(lhs.x - rhs.x) < epsilon &&
               std::abs(lhs.y - rhs.y) < epsilon &&
               std::abs(lhs.z - rhs.z) < epsilon;
    }
};
