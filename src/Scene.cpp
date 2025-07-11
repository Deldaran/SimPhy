#include "Scene.h"
#include "RaymarchRenderer.h"
#include <GL/glew.h>
#include <glm/glm.hpp>

Scene::Scene()
    : camera(5.0f, 45.0f, 30.0f), icosphere(1.0f, 3) {}

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
    // ...existing code...
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float light_pos[] = { 5.0f, 5.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    float ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    const auto& vertices = icosphere.getVertices();
    const auto& indices = icosphere.getIndices();
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            const auto& v = vertices[indices[i + j]];
            glNormal3fv(&v.normal.x);
            glVertex3fv(&v.position.x);
        }
    }
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_LIGHTING);
}

Camera& Scene::getCamera() {
    return camera;
}

Icosphere& Scene::getIcosphere() {
    return icosphere;
}

void Scene::regenerateIcosphere(float radius, int subdivisions) {
    icosphere = Icosphere(radius, subdivisions);
}
