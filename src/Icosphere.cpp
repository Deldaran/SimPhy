#include "Icosphere.h"
#include <map>
#include <cmath>
#include <array>

Icosphere::Icosphere(float radius, int subdivisions) : radius(radius) {
    createIcosphere(radius, subdivisions);
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
