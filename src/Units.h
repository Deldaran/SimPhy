#pragma once

// Système d'unités de la simulation, choisi pour que les grandeurs d'une
// galaxie soient des nombres « normaux » (ni 1e20, ni 1e-20) :
//
//   longueur : 1 kpc   (kiloparsec ≈ 3,086e19 m ≈ 3260 années-lumière)
//   temps    : 1 Myr   (million d'années)
//   masse    : 1e10 M☉ (dix milliards de masses solaires)
//
// La constante de gravitation s'en déduit :
//   G = 4,30091e-6 kpc·(km/s)²/M☉  et  1 km/s = 1,022712e-3 kpc/Myr
//   => G = 4,4985e-12 kpc³/(M☉·Myr²) = 0,044985 kpc³/(1e10 M☉·Myr²)
//
// Exemple : Voie lactée, v ≈ 220 km/s ≈ 0,225 kpc/Myr à R ≈ 8 kpc.
namespace Units {

constexpr float G = 0.044985f;

constexpr float KMS_PER_KPC_MYR = 977.79f; // 1 kpc/Myr = 977,79 km/s

inline float ToKms(float kpcPerMyr) { return kpcPerMyr * KMS_PER_KPC_MYR; }
inline float FromKms(float kms) { return kms / KMS_PER_KPC_MYR; }

} // namespace Units
