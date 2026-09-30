#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// Une particule telle qu'elle est stockée dans le SSBO (layout std430, 48 octets).
// Même structure que « struct Particle » dans les shaders : les deux doivent rester identiques.
struct Particle {
    glm::vec4 pos; // xyz = position (kpc),        w = masse (1e10 M☉)
    glm::vec4 vel; // xyz = vitesse (kpc/Myr),     w = inutilisé
    glm::vec4 acc; // xyz = accélération (kpc/Myr²), w = inutilisé
};
static_assert(sizeof(Particle) == 48, "Particle doit correspondre au layout std430 des shaders");

struct PhysicsParams {
    float centralMass = 10.0f; // masse centrale (1e10 M☉), 0 = désactivée
    float softening = 1.0f;    // longueur d'adoucissement ε (kpc)
    float dt = 0.5f;           // pas de temps (Myr), fixe
    int stepsPerFrame = 2;     // nombre de pas physiques par image affichée
};

// Particules sur GPU et intégration « leapfrog » (kick-drift-kick) en compute shaders.
class ParticleSystem {
public:
    bool Init();
    void Destroy();

    void Upload(const std::vector<Particle>& particles);
    void Step(const PhysicsParams& params);

    GLuint Buffer() const { return ssbo; }
    unsigned Count() const { return count; }

private:
    void ComputeAccelerations(const PhysicsParams& params);
    void Dispatch(GLuint prog);

    GLuint ssbo = 0;
    unsigned count = 0;
    bool accelerationsValid = false;

    GLuint kickProg = 0;
    GLuint driftProg = 0;
    GLuint accelCentralProg = 0;
};
