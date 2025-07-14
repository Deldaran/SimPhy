#version 450 core
in vec3 teColor;
in vec3 teNormal;
in vec3 tePosition;
out vec4 FragColor;

uniform vec3 lightDir; // Ajouté pour l'éclairage


float planetRadius = 6371000.0; // 1 unité = 1 m, planète taille réelle

float sphereSDF(vec3 p, float r) {
    // Relief plus marqué et océans plus profonds
    float relief = 0.12; // amplitude augmentée
    float height = relief * sin(10.0 * p.x / r) * sin(10.0 * p.y / r) * sin(10.0 * p.z / r);
    // Océans plus profonds
    if (height < 0.0) height *= 2.0;
    return length(p) - (r + height * r);
}

vec3 raymarchColor(vec3 p) {
    float d = sphereSDF(p, planetRadius);
    if (d < 0.01) {
        float h = length(p);
        if (h < planetRadius * 0.98) return vec3(0.0, 0.3, 1.0); // océan
        else if (h < planetRadius * 1.01) return vec3(0.1, 0.9, 0.1); // terre
        else if (h < planetRadius * 1.06) return vec3(0.7, 0.7, 0.7); // roche
        else return vec3(1.0, 1.0, 1.0); // neige
    }
    return vec3(0.1, 0.1, 0.2); // fond
}

float shadowMarch(vec3 p, vec3 ldir) {
    float t = 0.01;
    for (int i = 0; i < 64; ++i) {
        float d = sphereSDF(p + ldir * t, planetRadius);
        if (d < 0.001) return 0.0;
        t += d;
        if (t > planetRadius * 2.0) break;
    }
    return 1.0;
}

void main() {
    float shadow = 1.0; // ombre désactivée pour test
    float ambient = 0.20;
    float intensity = max(dot(normalize(teNormal), -normalize(lightDir)), 0.0);
    intensity = ambient + intensity * mix(0.3, 1.5, shadow);
    intensity = clamp(intensity, 0.0, 1.2);
    vec3 color = teColor * intensity;
    FragColor = vec4(color, 1.0);
}
