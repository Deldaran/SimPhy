#pragma once
#include "Camera.h"
#include "Icosphere.h"

class Scene {
public:
    Scene();
    void update(float deltaTime);
    void render(bool wireframe = false, bool useRaymarch = false);
    Camera& getCamera();
    Icosphere& getIcosphere();
    void regenerateIcosphere(float radius, int subdivisions);
private:
    Camera camera;
    Icosphere icosphere;
};
