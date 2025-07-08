#include "Planet.h"
#include <iostream>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constructeur de la planète - Initialise position, rayon, couleur et propriétés
Planet::Planet(const glm::vec3& position, float radius, const glm::vec3& color)
    : m_position(position), m_radius(radius), m_color(color),
      m_rotationAxis(0.0f, 1.0f, 0.0f), m_rotationSpeed(0.0f), m_currentRotation(0.0f),
      m_VAO(0), m_VBO(0), m_EBO(0), m_sphereVertexCount(0), m_sphereIndexCount(0), m_culledIndexCount(0),
      m_lastCameraPosition(0.0f), m_lodNeedsUpdate(true), m_lastLODUpdateTime(0.0f) {
}

// Destructeur de la planète - Libère les ressources
Planet::~Planet() {
    cleanup();
}

// Initialise la planète - Génère la géométrie et configure OpenGL
void Planet::initialize() {
    // Generate sphere geometry
    generateSphere();
    
    // Generate OpenGL objects
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    std::cout << "Planet initialized at position (" << m_position.x << ", " << m_position.y << ", " << m_position.z 
              << ") with radius " << m_radius << std::endl;
}

// Génère la géométrie sphérique avec système LOD dynamique
void Planet::generateSphere() {
    // Initialize with a basic icosahedron
    const float phi = (1.0f + sqrt(5.0f)) / 2.0f; // Golden ratio
    float norm = 1.0f / sqrt(phi * phi + 1.0f);
    
    // Create initial icosahedron vertices (12 vertices)
    m_vertices = {
        {-1 * norm, phi * norm, 0}, {1 * norm, phi * norm, 0}, 
        {-1 * norm, -phi * norm, 0}, {1 * norm, -phi * norm, 0},
        {0, -1 * norm, phi * norm}, {0, 1 * norm, phi * norm}, 
        {0, -1 * norm, -phi * norm}, {0, 1 * norm, -phi * norm},
        {phi * norm, 0, -1 * norm}, {phi * norm, 0, 1 * norm}, 
        {-phi * norm, 0, -1 * norm}, {-phi * norm, 0, 1 * norm}
    };
    
    // Scale vertices by radius
    for (auto& vertex : m_vertices) {
        vertex *= m_radius;
    }
    
    // Create initial icosahedron triangles
    std::vector<unsigned int> baseIndices = {
        // 5 faces around point 0
        0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10,  0, 10, 11,
        
        // 5 adjacent faces
        1, 5, 9,  5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1, 8,
        
        // 5 faces around point 3
        3, 9, 4,  3, 4, 2,  3, 2, 6,  3, 6, 8,  3, 8, 9,
        
        // 5 adjacent faces
        4, 9, 5,  2, 4, 11,  6, 2, 10,  8, 6, 7,  9, 8, 1
    };
    
    // Convert to Triangle structures
    m_triangles.clear();
    for (size_t i = 0; i < baseIndices.size(); i += 3) {
        Triangle triangle;
        triangle.v1 = baseIndices[i];
        triangle.v2 = baseIndices[i + 1];
        triangle.v3 = baseIndices[i + 2];
        triangle.center = getTriangleCenter(triangle);
        triangle.distanceToCamera = 0.0f;
        triangle.subdivisionLevel = 0;
        triangle.needsUpdate = true;
        triangle.isVisible = true;
        triangle.normal = calculateTriangleNormal(m_vertices[triangle.v1], m_vertices[triangle.v2], m_vertices[triangle.v3]);
        m_triangles.push_back(triangle);
    }
    
    // Force initial mesh rebuild
    m_lodNeedsUpdate = true;
}

// Effectue le rendu de la planète avec le système LOD et culling
void Planet::render(unsigned int shaderProgram, Camera* camera, float aspectRatio) {
    // Update LOD system
    updateDynamicLOD(camera);
    
    // Use shader program
    glUseProgram(shaderProgram);
    
    // Create transformation matrices
    glm::mat4 model = getModelMatrix();
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix(aspectRatio);
    
    // Set uniforms
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    // Perform culling to optimize rendering (after setting up matrices)
    performCulling(camera, aspectRatio);
    
    // Render only the visible triangles after culling
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_culledIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Met à jour la planète (rotation notamment)
void Planet::update(float deltaTime) {
    // Update rotation
    m_currentRotation += m_rotationSpeed * deltaTime;
    
    // Keep rotation in [0, 2π] range
    while (m_currentRotation > 2.0f * M_PI) {
        m_currentRotation -= 2.0f * M_PI;
    }
    while (m_currentRotation < 0.0f) {
        m_currentRotation += 2.0f * M_PI;
    }
}

// Nettoie et libère toutes les ressources OpenGL de la planète
void Planet::cleanup() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_EBO) {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }
}

// Modifie le rayon de la planète et régénère la géométrie si nécessaire
void Planet::setRadius(float radius) {
    if (radius != m_radius) {
        m_radius = radius;
        m_lodNeedsUpdate = true;
        generateSphere(); // Regenerate sphere with new radius
    }
}

// Met à jour le système LOD dynamique basé sur la distance à la caméra
void Planet::updateDynamicLOD(Camera* camera) {
    glm::vec3 cameraPos = camera->getPosition();
    
    // Limit LOD update frequency for optimization (max 30 FPS for LOD updates)
    float currentTime = glfwGetTime();
    float timeSinceLastUpdate = currentTime - m_lastLODUpdateTime;
    const float minUpdateInterval = 1.0f / 30.0f; // 30 FPS max
    
    // Check if camera moved significantly OR if planet is rotating
    float cameraMovement = glm::length(cameraPos - m_lastCameraPosition);
    bool cameraMovedSignificantly = cameraMovement > 0.05f;
    bool planetIsRotating = (m_rotationSpeed != 0.0f);
    bool timeForUpdate = timeSinceLastUpdate >= minUpdateInterval;
    
    // Update if camera moved, planet is rotating AND enough time passed, or mesh needs update
    if (!m_lodNeedsUpdate && !cameraMovedSignificantly && 
        (!planetIsRotating || !timeForUpdate)) {
        return; // No need to update
    }
    
    m_lastCameraPosition = cameraPos;
    m_lastLODUpdateTime = currentTime;
    bool needsRebuild = false;
    
    // Update distances and subdivision levels for all triangles
    for (auto& triangle : m_triangles) {
        // Calculate triangle center in world space, taking rotation into account
        glm::mat4 modelMatrix = getModelMatrix();
        glm::vec4 rotatedCenter = modelMatrix * glm::vec4(triangle.center, 1.0f);
        glm::vec3 worldCenter = glm::vec3(rotatedCenter);
        
        float newDistance = glm::length(cameraPos - worldCenter);
        
        // Only update if distance changed significantly (optimization)
        if (abs(newDistance - triangle.distanceToCamera) > 0.1f || m_lodNeedsUpdate) {
            triangle.distanceToCamera = newDistance;
            
            // Determine required subdivision level based on distance (logique ultra-précise)
            int requiredLevel = 0;
            float distance = triangle.distanceToCamera;
            
            // Logique ultra-agressive : beaucoup plus de niveaux pour atteindre 9000+ triangles
            if (distance < m_radius * 0.05f) {
                requiredLevel = 14; // Maximum absolu - 9000+ triangles
            } else if (distance < m_radius * 0.08f) {
                requiredLevel = 13; // Ultra haute précision - 7000+ triangles
            } else if (distance < m_radius * 0.12f) {
                requiredLevel = 12; // Très haute précision - 5000+ triangles
            } else if (distance < m_radius * 0.18f) {
                requiredLevel = 11; // Haute précision - 4000+ triangles
            } else if (distance < m_radius * 0.25f) {
                requiredLevel = 10; // Précision élevée - 3000+ triangles
            } else if (distance < m_radius * 0.35f) {
                requiredLevel = 9;  // Bonne précision - 2500+ triangles
            } else if (distance < m_radius * 0.5f) {
                requiredLevel = 8;  // Précision modérée - 2000+ triangles
            } else if (distance < m_radius * 1.0f) {
                requiredLevel = 7;  // Détail moyen-haut - 1500+ triangles
            } else if (distance < m_radius * 2.0f) {
                requiredLevel = 6;  // Détail moyen - 1200+ triangles
            } else if (distance < m_radius * 4.0f) {
                requiredLevel = 5;  // Bas détail - 1000+ triangles
            } else if (distance < m_radius * 8.0f) {
                requiredLevel = 4;  // Très bas détail
            } else if (distance < m_radius * 16.0f) {
                requiredLevel = 3;  // Détail minimal
            } else if (distance < m_radius * 32.0f) {
                requiredLevel = 2;  // Base
            } else if (distance < m_radius * 64.0f) {
                requiredLevel = 1;  // Lointain
            } else {
                requiredLevel = 0;  // Très lointain
            }
            
            // Check if subdivision level changed
            if (triangle.subdivisionLevel != requiredLevel) {
                triangle.subdivisionLevel = requiredLevel;
                triangle.needsUpdate = true;
                needsRebuild = true;
            }
        }
    }
    
    // Rebuild mesh if needed
    if (needsRebuild || m_lodNeedsUpdate) {
        rebuildMesh();
        m_lodNeedsUpdate = false;
    }
}

// Reconstruit le maillage de la planète selon les niveaux LOD
void Planet::rebuildMesh() {
    m_finalVertices.clear();
    m_finalIndices.clear();
    
    // Variables pour compter les subdivisions
    std::map<int, int> subdivisionCounts; // niveau -> nombre de triangles
    int totalTrianglesProcessed = 0;
    
    // Create an unordered_map to avoid duplicate vertices
    std::unordered_map<glm::vec3, int, Vec3Hash, Vec3Equal> vertexMap;
    std::vector<glm::vec3> finalVertexPositions;
    
    // Helper function to add vertex or get existing index with subdivision level tracking
    std::vector<int> vertexSubdivisionLevels;
    auto addVertex = [&](const glm::vec3& pos, int subdivisionLevel = 0) -> int {
        // Normalize position to sphere surface
        glm::vec3 normalizedPos = glm::normalize(pos) * m_radius;
        
        auto it = vertexMap.find(normalizedPos);
        if (it != vertexMap.end()) {
            // Update subdivision level to the maximum
            if (subdivisionLevel > vertexSubdivisionLevels[it->second]) {
                vertexSubdivisionLevels[it->second] = subdivisionLevel;
            }
            return it->second;
        }
        
        int index = static_cast<int>(finalVertexPositions.size());
        finalVertexPositions.push_back(normalizedPos);
        vertexSubdivisionLevels.push_back(subdivisionLevel);
        vertexMap[normalizedPos] = index;
        return index;
    };
    
    // Process each triangle with its subdivision level
    for (const auto& triangle : m_triangles) {
        totalTrianglesProcessed++;
        subdivisionCounts[triangle.subdivisionLevel]++;
        
        if (triangle.subdivisionLevel == 0) {
            // Use original triangle
            int v1 = addVertex(m_vertices[triangle.v1], 0);
            int v2 = addVertex(m_vertices[triangle.v2], 0);
            int v3 = addVertex(m_vertices[triangle.v3], 0);
            m_finalIndices.insert(m_finalIndices.end(), {
                static_cast<unsigned int>(v1), 
                static_cast<unsigned int>(v2), 
                static_cast<unsigned int>(v3)
            });
        } else {
            // Subdivide triangle
            subdivideTriangleRecursive(
                m_vertices[triangle.v1], 
                m_vertices[triangle.v2], 
                m_vertices[triangle.v3], 
                triangle.subdivisionLevel,
                triangle.subdivisionLevel,
                addVertex
            );
        }
    }
    
    // Build final vertex data with colors based on subdivision level
    m_finalVertices.clear();
    m_finalVertices.reserve(finalVertexPositions.size() * 9); // 3 position + 3 color + 3 normal
    
    for (size_t i = 0; i < finalVertexPositions.size(); ++i) {
        const auto& vertex = finalVertexPositions[i];
        int subdivisionLevel = vertexSubdivisionLevels[i];
        
        // Add position
        m_finalVertices.insert(m_finalVertices.end(), {vertex.x, vertex.y, vertex.z});
        
        // Add color based only on planet's base color (no LOD color modification)
        float normalizedHeight = (vertex.y / m_radius + 1.0f) * 0.5f; // 0 to 1
        glm::vec3 baseColor = m_color;
        
        // Simple height-based variation to add visual interest
        float r = baseColor.r * (0.8f + 0.2f * normalizedHeight);
        float g = baseColor.g * (0.8f + 0.2f * normalizedHeight);
        float b = baseColor.b * (0.8f + 0.2f * normalizedHeight);
        
        m_finalVertices.insert(m_finalVertices.end(), {r, g, b});
        
        // Add normal (normalized position for sphere)
        glm::vec3 normal = glm::normalize(vertex);
        m_finalVertices.insert(m_finalVertices.end(), {normal.x, normal.y, normal.z});
    }
    
    m_sphereVertexCount = static_cast<int>(finalVertexPositions.size());
    m_sphereIndexCount = static_cast<int>(m_finalIndices.size());
    
    // Log subdivision statistics
    std::cout << "Planet LOD Stats - Vertices: " << m_sphereVertexCount 
              << ", Triangles: " << (m_sphereIndexCount / 3) << std::endl;
    std::cout << "Subdivision levels: ";
    for (const auto& pair : subdivisionCounts) {
        std::cout << "L" << pair.first << ":" << pair.second << " ";
    }
    std::cout << std::endl;
    
    // Update OpenGL buffers
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_finalVertices.size() * sizeof(float), m_finalVertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_finalIndices.size() * sizeof(unsigned int), m_finalIndices.data(), GL_DYNAMIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Subdivise récursivement un triangle pour augmenter le détail
void Planet::subdivideTriangleRecursive(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, 
                                       int level, int subdivisionLevel, std::function<int(const glm::vec3&, int)> addVertex) {
    if (level <= 0) {
        // Base case: add the triangle
        int i1 = addVertex(v1, subdivisionLevel);
        int i2 = addVertex(v2, subdivisionLevel);
        int i3 = addVertex(v3, subdivisionLevel);
        m_finalIndices.insert(m_finalIndices.end(), {
            static_cast<unsigned int>(i1), 
            static_cast<unsigned int>(i2), 
            static_cast<unsigned int>(i3)
        });
        return;
    }
    
    // Calculate midpoints and normalize to sphere surface
    glm::vec3 m1 = glm::normalize(v1 + v2) * m_radius;
    glm::vec3 m2 = glm::normalize(v2 + v3) * m_radius;
    glm::vec3 m3 = glm::normalize(v3 + v1) * m_radius;
    
    // Recursively subdivide the 4 new triangles
    subdivideTriangleRecursive(v1, m1, m3, level - 1, subdivisionLevel, addVertex);
    subdivideTriangleRecursive(v2, m2, m1, level - 1, subdivisionLevel, addVertex);
    subdivideTriangleRecursive(v3, m3, m2, level - 1, subdivisionLevel, addVertex);
    subdivideTriangleRecursive(m1, m2, m3, level - 1, subdivisionLevel, addVertex);
}

// Calcule le centre d'un triangle
glm::vec3 Planet::getTriangleCenter(const Triangle& triangle) {
    return (m_vertices[triangle.v1] + m_vertices[triangle.v2] + m_vertices[triangle.v3]) / 3.0f;
}

// Calcule la distance entre un point et la caméra
float Planet::getDistanceToCamera(const glm::vec3& point, Camera* camera) {
    glm::vec3 worldPoint = point + m_position;
    return glm::length(camera->getPosition() - worldPoint);
}

// Vérifie si un point est à l'intérieur de la planète
bool Planet::isPointInside(const glm::vec3& point) const {
    float distance = glm::length(point - m_position);
    return distance < m_radius;
}

// Calcule la distance d'un point à la surface de la planète
float Planet::getDistanceToSurface(const glm::vec3& point) const {
    float distanceToCenter = glm::length(point - m_position);
    return distanceToCenter - m_radius;
}

// Empêche la pénétration d'un point dans la planète avec une marge configurable
glm::vec3 Planet::preventPenetration(const glm::vec3& point, float margin) const {
    float distanceToCenter = glm::length(point - m_position);
    // Utilise la marge passée en paramètre
    float minDistance = m_radius + margin;
    
    if (distanceToCenter < minDistance) {
        // Calculate direction from planet center to point
        glm::vec3 direction = point - m_position;
        
        // Handle case where point is exactly at planet center
        if (glm::length(direction) < 0.0001f) {
            direction = glm::vec3(0.0f, 0.0f, 1.0f); // Default direction
        }
        
        // Normalize and scale to minimum distance
        direction = glm::normalize(direction) * minDistance;
        
        // Return corrected position
        return m_position + direction;
    }
    
    return point; // No correction needed
}

// Calcule le multiplicateur de vitesse basé sur la proximité de la surface - système pour approche très proche
float Planet::getSpeedMultiplierNearSurface(const glm::vec3& point) const {
    float distanceToSurface = getDistanceToSurface(point);
    
    // Zones de réduction de vitesse pour permettre l'approche ultra-proche
    const float ultraCloseZone = 0.01f;   // Zone ultra-proche à 0.01 unité
    const float veryCloseZone = 0.1f;     // Zone très proche à 0.1 unité  
    const float closeZone = 1.0f;         // Zone proche à 1 unité
    const float mediumZone = 10.0f;       // Zone moyenne à 10 unités
    
    const float ultraSlowMultiplier = 0.001f; // 0.1% de la vitesse normale
    const float verySlowMultiplier = 0.01f;   // 1% de la vitesse normale
    const float slowMultiplier = 0.05f;       // 5% de la vitesse normale
    const float mediumMultiplier = 0.2f;      // 20% de la vitesse normale
    
    if (distanceToSurface <= 0.0f) {
        // Inside the planet - ultra slow
        return ultraSlowMultiplier;
    } else if (distanceToSurface <= ultraCloseZone) {
        // Ultra close to surface (moins de 0.01 unité)
        return ultraSlowMultiplier;
    } else if (distanceToSurface <= veryCloseZone) {
        // Very close to surface (moins de 0.1 unité)
        float t = distanceToSurface / veryCloseZone;
        return ultraSlowMultiplier + (verySlowMultiplier - ultraSlowMultiplier) * t;
    } else if (distanceToSurface <= closeZone) {
        // Close to surface (moins de 1 unité)
        float t = distanceToSurface / closeZone;
        return verySlowMultiplier + (slowMultiplier - verySlowMultiplier) * t;
    } else if (distanceToSurface <= mediumZone) {
        // Medium distance (moins de 10 unités)
        float t = distanceToSurface / mediumZone;
        return slowMultiplier + (mediumMultiplier - slowMultiplier) * t;
    } else {
        // Far from surface - normal speed
        return 1.0f;
    }
}

// Retourne la matrice de transformation de la planète (position + rotation)
glm::mat4 Planet::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    
    // Apply translation
    model = glm::translate(model, m_position);
    
    // Apply rotation
    if (m_rotationSpeed != 0.0f) {
        model = glm::rotate(model, m_currentRotation, m_rotationAxis);
    }
    
    return model;
}

// Effectue le culling des triangles pour optimiser le rendu
void Planet::performCulling(Camera* camera, float aspectRatio) {
    m_culledIndices.clear();
    
    int totalTriangles = 0;
    int culledTriangles = 0;
    int backfaceCulled = 0;
    int frustumCulled = 0;
    
    glm::vec3 cameraPos = camera->getPosition();
    glm::mat4 modelMatrix = getModelMatrix();
    
    // Process triangles in groups of 3 indices (one triangle)
    for (size_t i = 0; i < m_finalIndices.size(); i += 3) {
        totalTriangles++;
        
        // Get vertex indices
        unsigned int i1 = m_finalIndices[i];
        unsigned int i2 = m_finalIndices[i + 1];
        unsigned int i3 = m_finalIndices[i + 2];
        
        // Convert vertex buffer indices to positions (each vertex has 6 floats: xyz + rgb)
        glm::vec3 v1(m_finalVertices[i1 * 9], m_finalVertices[i1 * 9 + 1], m_finalVertices[i1 * 9 + 2]);
        glm::vec3 v2(m_finalVertices[i2 * 9], m_finalVertices[i2 * 9 + 1], m_finalVertices[i2 * 9 + 2]);
        glm::vec3 v3(m_finalVertices[i3 * 9], m_finalVertices[i3 * 9 + 1], m_finalVertices[i3 * 9 + 2]);
        
        // Transform vertices to world space
        glm::vec4 worldV1 = modelMatrix * glm::vec4(v1, 1.0f);
        glm::vec4 worldV2 = modelMatrix * glm::vec4(v2, 1.0f);
        glm::vec4 worldV3 = modelMatrix * glm::vec4(v3, 1.0f);
        
        glm::vec3 wv1 = glm::vec3(worldV1);
        glm::vec3 wv2 = glm::vec3(worldV2);
        glm::vec3 wv3 = glm::vec3(worldV3);
        
        // Backface culling - vérifier si le triangle fait face à la caméra
        if (isBackfacing(wv1, wv2, wv3, cameraPos)) {
            backfaceCulled++;
            culledTriangles++;
            continue;
        }
        
        // Frustum culling - vérifier si le triangle est dans le champ de vision
        if (!isTriangleInFrustum(wv1, wv2, wv3, camera, aspectRatio)) {
            frustumCulled++;
            culledTriangles++;
            continue;
        }
        
        // Triangle is visible, add to culled indices
        m_culledIndices.push_back(i1);
        m_culledIndices.push_back(i2);
        m_culledIndices.push_back(i3);
    }
    
    m_culledIndexCount = static_cast<int>(m_culledIndices.size());
    
    // Update OpenGL Element Buffer with culled indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_culledIndices.size() * sizeof(unsigned int), 
                 m_culledIndices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Teste si un triangle fait face à la caméra (backface culling)
bool Planet::isBackfacing(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& cameraPos) {
    // Calcule la normale du triangle
    glm::vec3 normal = calculateTriangleNormal(v1, v2, v3);
    
    // Calcule le centre du triangle
    glm::vec3 triangleCenter = (v1 + v2 + v3) / 3.0f;
    
    // Vecteur du centre du triangle vers la caméra
    glm::vec3 toCamera = glm::normalize(cameraPos - triangleCenter);
    
    // Si le produit scalaire est négatif, le triangle fait face à l'opposé de la caméra
    float dotProduct = glm::dot(normal, toCamera);
    
    // Amélioration : utiliser un seuil pour éviter les problèmes de précision numérique
    // et pour être plus agressif avec le culling des faces arrière
    return dotProduct < -0.01f; // Seuil légèrement négatif pour plus d'agressivité
}

// Teste si un triangle est dans le frustum de la caméra (frustum culling)
bool Planet::isTriangleInFrustum(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, Camera* camera, float aspectRatio) {
    glm::vec3 cameraPos = camera->getPosition();
    glm::vec3 cameraFront = camera->getFront();
    
    // Calculer les matrices de la caméra
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix(aspectRatio);
    glm::mat4 viewProjection = projection * view;
    
    // Transformer chaque vertex du triangle en espace de clip
    glm::vec4 clipV1 = viewProjection * glm::vec4(v1, 1.0f);
    glm::vec4 clipV2 = viewProjection * glm::vec4(v2, 1.0f);
    glm::vec4 clipV3 = viewProjection * glm::vec4(v3, 1.0f);
    
    // Test simple : si tous les vertices sont derrière la caméra (z négatif en view space)
    glm::vec4 viewV1 = view * glm::vec4(v1, 1.0f);
    glm::vec4 viewV2 = view * glm::vec4(v2, 1.0f);
    glm::vec4 viewV3 = view * glm::vec4(v3, 1.0f);
    
    if (viewV1.z > 0.0f && viewV2.z > 0.0f && viewV3.z > 0.0f) {
        return false; // Triangle complètement derrière la caméra
    }
    
    // Diviser par w pour obtenir les coordonnées normalisées (NDC)
    if (clipV1.w > 0.0001f) clipV1 /= clipV1.w;
    if (clipV2.w > 0.0001f) clipV2 /= clipV2.w;
    if (clipV3.w > 0.0001f) clipV3 /= clipV3.w;
    
    // Test de frustum : vérifier si au moins un vertex est dans le frustum [-1, 1] pour x, y, z
    auto isVertexInFrustum = [](const glm::vec4& v) -> bool {
        return v.x >= -1.0f && v.x <= 1.0f &&
               v.y >= -1.0f && v.y <= 1.0f &&
               v.z >= -1.0f && v.z <= 1.0f;
    };
    
    // Si au moins un vertex est visible, considérer le triangle comme potentiellement visible
    if (isVertexInFrustum(clipV1) || isVertexInFrustum(clipV2) || isVertexInFrustum(clipV3)) {
        return true;
    }
    
    // Test supplémentaire : vérifier si le triangle traverse le frustum
    // Calculer les limites du triangle en NDC
    float minX = std::min(clipV1.x, std::min(clipV2.x, clipV3.x));
    float maxX = std::max(clipV1.x, std::max(clipV2.x, clipV3.x));
    float minY = std::min(clipV1.y, std::min(clipV2.y, clipV3.y));
    float maxY = std::max(clipV1.y, std::max(clipV2.y, clipV3.y));
    
    // Si le triangle englobe le frustum, il est visible
    if (minX <= -1.0f && maxX >= 1.0f && minY <= -1.0f && maxY >= 1.0f) {
        return true;
    }
    
    // Si les limites intersectent le frustum, le triangle peut être visible
    return !(maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f);
}

// Calcule la normale d'un triangle
glm::vec3 Planet::calculateTriangleNormal(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;
    glm::vec3 normal = glm::cross(edge1, edge2);
    
    // Normaliser si pas nul
    if (glm::length(normal) > 0.0001f) {
        normal = glm::normalize(normal);
    } else {
        // Fallback : utiliser la normale moyenne des vertices (pour les sphères)
        // Car les vertices d'une sphère pointent vers l'extérieur depuis le centre
        glm::vec3 center = m_position; // Centre de la planète
        glm::vec3 avgVertex = (v1 + v2 + v3) / 3.0f;
        normal = glm::normalize(avgVertex - center);
    }
    
    return normal;
}
