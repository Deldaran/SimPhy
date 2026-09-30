#include "ParticleSystem.h"

#include "Shader.h"
#include "Units.h"

namespace {
constexpr unsigned WORKGROUP_SIZE = 256; // doit correspondre à local_size_x dans les .comp
}

bool ParticleSystem::Init() {
    kickProg = LoadComputeProgram("kick.comp");
    driftProg = LoadComputeProgram("drift.comp");
    accelCentralProg = LoadComputeProgram("accel_central.comp");
    glGenBuffers(1, &ssbo);
    return kickProg && driftProg && accelCentralProg;
}

void ParticleSystem::Destroy() {
    glDeleteBuffers(1, &ssbo);
    glDeleteProgram(kickProg);
    glDeleteProgram(driftProg);
    glDeleteProgram(accelCentralProg);
}

void ParticleSystem::Upload(const std::vector<Particle>& particles) {
    count = static_cast<unsigned>(particles.size());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    accelerationsValid = false;
}

// Un pas de leapfrog KDK (kick-drift-kick) :
//   1. kick  : v(t + dt/2) = v(t) + a(t) · dt/2
//   2. drift : x(t + dt)   = x(t) + v(t + dt/2) · dt
//   3. accélérations a(t + dt) calculées aux nouvelles positions
//   4. kick  : v(t + dt)   = v(t + dt/2) + a(t + dt) · dt/2
// Le schéma est symplectique : l'énergie oscille légèrement mais ne dérive pas,
// contrairement à Euler, où les orbites spiralent au bout de quelques tours.
void ParticleSystem::Step(const PhysicsParams& params) {
    if (count == 0) return;
    if (!accelerationsValid) {
        ComputeAccelerations(params);
        accelerationsValid = true;
    }

    SetUniform(kickProg, "halfDt", 0.5f * params.dt);
    SetUniform(driftProg, "dt", params.dt);

    Dispatch(kickProg);
    Dispatch(driftProg);
    ComputeAccelerations(params);
    Dispatch(kickProg);
}

void ParticleSystem::ComputeAccelerations(const PhysicsParams& params) {
    // Phase 0 : seule force = masse centrale adoucie.
    // Les phases suivantes ajouteront ici l'auto-gravité (sommation directe, puis Particle-Mesh).
    SetUniform(accelCentralProg, "G", Units::G);
    SetUniform(accelCentralProg, "centralMass", params.centralMass);
    SetUniform(accelCentralProg, "softening", params.softening);
    Dispatch(accelCentralProg);
}

void ParticleSystem::Dispatch(GLuint prog) {
    SetUniform(prog, "count", count);
    glUseProgram(prog);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glDispatchCompute((count + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);
    // Le dispatch suivant (ou le rendu) lit ce que celui-ci vient d'écrire :
    // la barrière garantit que toutes les écritures sont visibles avant.
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
