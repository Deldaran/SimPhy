
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <map>
#include <utility>

class Icosphere {
public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        float height = 0.0f; // Hauteur procédurale
        glm::vec3 color = glm::vec3(1.0f); // Couleur du sommet
    };

    Icosphere(float radius = 1.0f, int subdivisions = 5); // Par défaut, subdivisions élevées
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }

    // Applique un relief procédural (océans et montagnes)
    void applyProceduralTerrain(float oceanLevel = 0.0f, float mountainHeight = 0.3f);

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float radius;
    void createIcosphere(float radius, int subdivisions);
    unsigned int addVertex(const glm::vec3& position);
    glm::vec3 getMiddlePoint(unsigned int p1, unsigned int p2, std::map<std::pair<unsigned int, unsigned int>, unsigned int>& cache);
};
