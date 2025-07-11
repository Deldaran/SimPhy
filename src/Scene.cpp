#include "Scene.h"
#include "RaymarchRenderer.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

Scene::Scene()
    : camera(63710.0f, 45.0f, 30.0f), icosphere(6371.0f, 6) {
    // Rayon Terre = 6371 km
    icosphere.applyProceduralTerrain(0.4f, 200.0f); // Relief adapté à l'échelle Terre
}


void Scene::update(float deltaTime) {
    // Camera update logic if needed
}

void Scene::render(bool wireframe, bool useRaymarch) {
    if (useRaymarch) {
        static RaymarchRenderer renderer;
        glm::vec3 camPos = camera.getPosition();
        glm::mat3 camRot = glm::mat3(1.0f); // TODO: build from camera orientation
        float sphereRadius = icosphere.getVertices().empty() ? 1.0f : glm::length(icosphere.getVertices()[0].position);
        glm::vec3 sphereCenter(0.0f);
        glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
        int winWidth = 1280, winHeight = 720;
        renderer.render(camPos, camRot, sphereRadius, sphereCenter, lightDir, winWidth, winHeight);
        return;
    }
    // Rendu mesh sans éclairage, couleur brute du sommet
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    const auto& vertices = icosphere.getVertices();
    const auto& indices = icosphere.getIndices();
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            const auto& v = vertices[indices[i + j]];
            glColor3fv(&v.color.x); // Utilise la couleur du sommet
            glVertex3fv(&v.position.x);
        }
    }
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

Camera& Scene::getCamera() {
    return camera;
}

Icosphere& Scene::getIcosphere() {
    return icosphere;
}

void Scene::regenerateIcosphere(float radius, int subdivisions) {
    icosphere = Icosphere(radius, subdivisions);
    icosphere.applyProceduralTerrain(0.4f, 20.0f); // Relief adapté à l'échelle
    camera = Camera(radius * 1.5f, camera.getYaw(), camera.getPitch()); // Place la caméra plus loin
}
