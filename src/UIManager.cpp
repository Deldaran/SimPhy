#include "UIManager.h"


UIManager::UIManager(Scene& scene)
    : scene(scene) {
    syncWithScene();
}

void UIManager::syncWithScene() {
    radius = scene.getIcosphere().getVertices().empty() ? 1.0f : glm::length(scene.getIcosphere().getVertices()[0].position);
    subdivisions = 3;
}

// UIManager::renderUI() supprimé
