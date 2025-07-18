// Includes standards
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "stb_image_write.h"


#include "Icosphere.h"
#include <map>
#include <cmath>
#include <array>


#include "FastNoiseLite.h"

// Génère une texture d'altitude sphérique (longitude/latitude) et la sauvegarde en PNG
static void saveHeightmapTexture(const std::vector<Icosphere::Vertex>& vertices, int texSize, const std::string& filename) {
    std::vector<unsigned char> image(texSize * texSize);
    for (int y = 0; y < texSize; ++y) {
        float v = float(y) / float(texSize - 1);
        for (int x = 0; x < texSize; ++x) {
            float u = float(x) / float(texSize - 1);
            // Cherche le sommet le plus proche (brut, mais suffisant pour une première version)
            float minDist = 1e9;
            float bestAlt = 0.0f;
            for (const auto& vert : vertices) {
                float du = abs(vert.uv.x - u);
                float dv = abs(vert.uv.y - v);
                float d = du*du + dv*dv;
                if (d < minDist) { minDist = d; bestAlt = vert.uv.y; }
            }
            image[y * texSize + x] = (unsigned char)(255.0f * bestAlt);
        }
    }
    stbi_write_png(filename.c_str(), texSize, texSize, 1, image.data(), texSize);
}

Icosphere::Icosphere(const glm::vec3& center, float radius, int subdivisions, bool withRelief)
    : center(center), radius(radius), withRelief(withRelief) {
    createIcosphere(radius, subdivisions);
    if (withRelief) {
        // Génère les reliefs procéduraux automatiquement
        applyProceduralTerrain(0.48f, radius * 0.2f);
    }
    // Décale tous les sommets pour que le mesh soit centré sur center
    for (auto& v : vertices) {
        v.position += center;
        // UV sphérique (ne modifie que x, y = altitude déjà encodée)
        glm::vec3 p = glm::normalize(v.position - center);
        float u = 0.5f + atan2(p.z, p.x) / (2.0f * 3.14159265358979323846f);
        v.uv.x = u;
        // v.uv.y = altitude déjà encodée dans applyProceduralTerrain
    }
    // Génère la texture d'altitude (par défaut 1024x1024)
    if (withRelief) {
        printf("[Icosphere] Début génération heightmap...\n");
        saveHeightmapTexture(vertices, 256, "heightmap.png");
        printf("[Icosphere] Heightmap généré et sauvegardé.\n");
    }
}



void Icosphere::initGLBuffers() {
    if (vao != 0) return; // Déjà initialisé
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // Attributs: position (0), normal (1), uv (2), couleur (3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glBindVertexArray(0);
}

void Icosphere::bindGLBuffers() const {
    glBindVertexArray(vao);
}

void Icosphere::draw(bool triangles) const {
    glBindVertexArray(vao);
    if (triangles) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    } else {
        glPatchParameteri(GL_PATCH_VERTICES, 3);
        glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void Icosphere::cleanupGLBuffers() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    vao = vbo = ebo = 0;
}

void Icosphere::applyProceduralTerrain(float oceanLevel, float mountainHeight) {
    // Bruit global pour la map eau/terre
    FastNoiseLite globalNoise;
    globalNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    globalNoise.SetFrequency(0.70f); // Très basse fréquence, continents

    // Bruit terrain pour reliefs (montagnes, plaines, forêts)
    FastNoiseLite terrainNoise;
    terrainNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    terrainNoise.SetFrequency(2.0f); // Moyenne fréquence

    // Bruit fin pour détails (roches, forêts, etc.)
    FastNoiseLite detailNoise;
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    detailNoise.SetFrequency(8.0f); // Haute fréquence

    // Palettes
    std::vector<glm::vec3> waterPalette = {
        {0.0f, 0.2f, 0.6f}, {0.0f, 0.3f, 0.8f}, {0.0f, 0.4f, 1.0f}, {0.0f, 0.5f, 1.0f}, {0.1f, 0.7f, 1.0f}
    };
    std::vector<glm::vec3> landPalette = {
        {0.5f, 0.8f, 0.7f}, {1.0f, 0.95f, 0.5f}, {0.8f, 0.9f, 0.4f}, {0.6f, 0.8f, 0.3f}, {0.3f, 0.7f, 0.2f},
        {0.1f, 0.9f, 0.1f}, {0.0f, 0.5f, 0.0f}, {0.5f, 0.3f, 0.1f}, {0.7f, 0.7f, 0.7f}, {0.4f, 0.4f, 0.5f},
        {0.7f, 0.8f, 0.9f}, {0.9f, 0.95f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.8f, 0.8f, 0.8f}, {0.6f, 0.7f, 0.9f}
    };

    float oceanThreshold = 0.48f; // Seuil pour l'eau
    float mountainAmplitude = mountainHeight; // Amplitude max montagne (ex: 2000m)
    float detailAmplitude = mountainHeight * 0.2f; // Détail fin

    for (auto& v : vertices) {
        glm::vec3 posNorm = glm::normalize(v.position);
        float global = globalNoise.GetNoise(posNorm.x * 10.0f, posNorm.y * 10.0f, posNorm.z * 10.0f);
        global = (global + 1.0f) * 0.5f;
        float terrain = terrainNoise.GetNoise(posNorm.x * 100.0f, posNorm.y * 100.0f, posNorm.z * 100.0f);
        terrain = (terrain + 1.0f) * 0.5f;
        float detail = detailNoise.GetNoise(posNorm.x * 500.0f, posNorm.y * 500.0f, posNorm.z * 500.0f);
        detail = (detail + 1.0f) * 0.5f;

        float h = 0.0f;
        glm::vec3 color;
        float altitude = 0.0f;
        if (global < oceanThreshold) {
            // Eau
            float idxf = global * (waterPalette.size() - 1);
            int idx = static_cast<int>(idxf);
            float t = idxf - idx;
            color = glm::mix(waterPalette[idx], waterPalette[std::min(idx+1, (int)waterPalette.size()-1)], t);
            h = -100.0f * (oceanThreshold - global); // Profondeur max 100m
            altitude = -100.0f * (oceanThreshold - global);
        } else {
            // Terre
            float idxf = terrain * (landPalette.size() - 1);
            int idx = static_cast<int>(idxf);
            float t = idxf - idx;
            color = glm::mix(landPalette[idx], landPalette[std::min(idx+1, (int)landPalette.size()-1)], t);
            float mountain = pow(terrain, 3.0f) * mountainAmplitude; // Montagnes max
            float detailRelief = (detail - 0.5f) * detailAmplitude; // Détail fin
            h = mountain + detailRelief;
            altitude = h;
        }
        v.color = color;
        v.position = posNorm * (radius + h);
        v.normal = posNorm;
        // Encode l'altitude dans le canal UV.y (ou un canal custom si Vertex le permet)
        v.uv.y = altitude / mountainHeight; // Normalisé [-1,1] ou [0,1] selon usage
    }
}

unsigned int Icosphere::addVertex(const glm::vec3& position) {
    Vertex v;
    v.position = glm::normalize(position) * radius;
    v.normal = glm::normalize(position);
    vertices.push_back(v);
    return static_cast<unsigned int>(vertices.size() - 1);
}

glm::vec3 Icosphere::getMiddlePoint(unsigned int p1, unsigned int p2, std::map<std::pair<unsigned int, unsigned int>, unsigned int>& cache) {
    auto smallerIndex = std::min(p1, p2);
    auto greaterIndex = std::max(p1, p2);
    auto key = std::make_pair(smallerIndex, greaterIndex);
    if (cache.find(key) != cache.end()) {
        return vertices[cache[key]].position;
    }
    glm::vec3 point1 = vertices[p1].position;
    glm::vec3 point2 = vertices[p2].position;
    glm::vec3 middle = glm::normalize((point1 + point2) * 0.5f) * radius;
    unsigned int i = addVertex(middle);
    cache[key] = i;
    return middle;
}

void Icosphere::createIcosphere(float radius, int subdivisions) {
    this->radius = radius;
    vertices.clear();
    indices.clear();
    // Create 12 vertices of a icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::vector<glm::vec3> positions = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };
    for (auto& pos : positions) addVertex(pos);
    // 20 faces
    std::vector<std::array<unsigned int, 3>> faces = {
        {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
        {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
        {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
        {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1}
    };
    // Subdivide faces
    for (int i = 0; i < subdivisions; ++i) {
        std::map<std::pair<unsigned int, unsigned int>, unsigned int> cache;
        std::vector<std::array<unsigned int, 3>> newFaces;
        for (auto& tri : faces) {
            unsigned int a = addVertex(getMiddlePoint(tri[0], tri[1], cache));
            unsigned int b = addVertex(getMiddlePoint(tri[1], tri[2], cache));
            unsigned int c = addVertex(getMiddlePoint(tri[2], tri[0], cache));
            newFaces.push_back({tri[0], a, c});
            newFaces.push_back({tri[1], b, a});
            newFaces.push_back({tri[2], c, b});
            newFaces.push_back({a, b, c});
        }
        faces = newFaces;
    }
    // Store indices
    for (auto& tri : faces) {
        indices.push_back(tri[0]);
        indices.push_back(tri[1]);
        indices.push_back(tri[2]);
    }
}

glm::vec3 Icosphere::getCenter() const {
    return center;
}