#pragma once

#include "ParticleSystem.h"

#include <cstdint>
#include <vector>

struct DiskParams {
    int count = 1 << 18;              // nombre de particules
    float diskMass = 5.0f;            // masse totale du disque (1e10 M☉), Voie lactée ≈ 5
    float scaleLength = 3.0f;         // longueur d'échelle R_d (kpc), Voie lactée ≈ 2,6 à 3,5
    float maxRadius = 20.0f;          // troncature du disque (kpc)
    float scaleHeight = 0.3f;         // épaisseur verticale σ_z (kpc)
    float velocityDispersion = 0.01f; // dispersion des vitesses σ_v (kpc/Myr), 0,01 ≈ 10 km/s
    uint32_t seed = 42;               // graine aléatoire : même graine => même galaxie
};

// Vitesse circulaire autour de la masse centrale adoucie :
//   v_c² = R · |a(R)| = G M R² / (R² + ε²)^(3/2)
float CircularVelocity(float radius, const PhysicsParams& physics);

// Disque exponentiel en rotation circulaire autour de la masse centrale.
std::vector<Particle> MakeExponentialDisk(const DiskParams& disk, const PhysicsParams& physics);
