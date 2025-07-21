#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 cameraPos;

// Métallique : couleur de base gris acier
const vec3 baseColor = vec3(0.5, 0.5, 0.55);
const float metallic = 1.0;
const float roughness = 0.2;

void main() {
    // Normalisé
    vec3 N = normalize(Normal);
    vec3 L = normalize(sunDirection);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-L, N);

    // Lambert
    float diff = max(dot(N, L), 0.0);
    // Spéculaire simple (Blinn-Phong)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0) * metallic;

    // Couleur finale
    vec3 color = baseColor * diff * sunColor + vec3(0.7) * spec * sunColor;
    // Ambiante faible
    color += baseColor * 0.08;
    FragColor = vec4(color, 1.0);
}
