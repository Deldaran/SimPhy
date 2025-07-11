#pragma once
#include "Scene.h"

class UIManager {
public:
    UIManager(Scene& scene);
    void renderUI();
    void syncWithScene();
private:
    Scene& scene;
    float radius;
    int subdivisions;
};
