#include "InitialConditions.h"

#include "Units.h"

#include <cmath>
#include <random>

float CircularVelocity(float radius, const PhysicsParams& physics) {
    float r2 = radius * radius;
    float d2 = r2 + physics.softening * physics.softening;
    return std::sqrt(Units::G * physics.centralMass * r2 / (d2 * std::sqrt(d2)));
}

std::vector<Particle> MakeExponentialDisk(const DiskParams& disk, const PhysicsParams& physics) {
    std::mt19937 rng(disk.seed);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
    std::normal_distribution<float> heightDist(0.0f, disk.scaleHeight);
    std::normal_distribution<float> velocityNoise(0.0f, disk.velocityDispersion);

    // Densité de surface exponentielle Σ(R) ∝ exp(-R/R_d), celle observée dans les galaxies spirales.
    // Le nombre de particules entre R et R+dR vaut 2πR·Σ(R)·dR ∝ R·exp(-R/R_d) :
    // c'est exactement une loi Gamma de forme 2 et d'échelle R_d.
    std::gamma_distribution<float> radiusDist(2.0f, disk.scaleLength);

    const float particleMass = disk.diskMass / disk.count;

    std::vector<Particle> particles(disk.count);
    for (Particle& p : particles) {
        float radius;
        do {
            radius = radiusDist(rng);
        } while (radius > disk.maxRadius);

        float angle = angleDist(rng);
        float c = std::cos(angle);
        float s = std::sin(angle);

        p.pos = glm::vec4(radius * c, radius * s, heightDist(rng), particleMass);

        // Orbite circulaire dans le sens direct (autour de +z), plus une agitation aléatoire.
        float vc = CircularVelocity(radius, physics);
        p.vel = glm::vec4(-s * vc + velocityNoise(rng),
                           c * vc + velocityNoise(rng),
                           velocityNoise(rng),
                           0.0f);
        p.acc = glm::vec4(0.0f);
    }
    return particles;
}
