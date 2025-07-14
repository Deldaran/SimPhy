
#pragma once
#include "Camera.h"
#include "Icosphere.h"
#include <string>

class Scene {
public:
    // Ajoute une liste d'objets sélectionnables (ici juste la planète, mais extensible)
    struct Selectable {
        glm::vec3 center;
        std::string name;
    };
    std::vector<Selectable> objects;
    int selectedIndex = 0;
    Scene();
    void update(float deltaTime);
    void render(bool wireframe = false);
    Camera& getCamera();
    Icosphere& getIcosphere();
    void regenerateIcosphere(float radius, int subdivisions);
private:
    Camera camera;
    Icosphere icosphere;
    GLuint icosphereProgram = 0;
};
