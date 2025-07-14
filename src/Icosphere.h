
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>
#include <utility>


class Icosphere {
public:
    Icosphere(const glm::vec3& center = glm::vec3(0.0f), float radius = 6371.0f, int subdivisions = 9);
    glm::vec3 getCenter() const;
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        float height = 0.0f; // Hauteur procédurale
        glm::vec3 color = glm::vec3(1.0f); // Couleur du sommet
    };

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }

    // Applique un relief procédural (océans et montagnes)
    void applyProceduralTerrain(float oceanLevel = 0.0f, float mountainHeight = 0.3f);

    // Initialisation et gestion des buffers OpenGL
    void initGLBuffers();
    void bindGLBuffers() const;
    void draw() const;
    void cleanupGLBuffers();

private:
    glm::vec3 center; // Position centrale de l'icosphere
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float radius;
    void createIcosphere(float radius, int subdivisions);
    unsigned int addVertex(const glm::vec3& position);
    glm::vec3 getMiddlePoint(unsigned int p1, unsigned int p2, std::map<std::pair<unsigned int, unsigned int>, unsigned int>& cache);

    // Buffers OpenGL
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
};
