
#pragma once
#include "Camera.h"
#include "Icosphere.h"
#include <glad/glad.h>

class Scene {
public:
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
