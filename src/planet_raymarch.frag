#version 450 core
out vec4 FragColor;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 camPos;
uniform vec3 lightDir;
uniform vec2 iResolution;

// SDF pour une planète avec reliefs
float sphereSDF(vec3 p, float r) {
    // Ajoute un relief simple (montagnes)
    float height = 0.05 * sin(10.0 * p.x) * sin(10.0 * p.y) * sin(10.0 * p.z);
    return length(p) - (r + height);
}

// Ray marching principal
float raymarch(vec3 ro, vec3 rd, out vec3 pHit) {
    float t = 0.0;
    for (int i = 0; i < 128; ++i) {
        vec3 p = ro + rd * t;
        float d = sphereSDF(p, 1.0);
        if (d < 0.001) {
            pHit = p;
            return t;
        }
        t += d;
        if (t > 100.0) break;
    }
    return -1.0;
}

// Shadow marching
float shadowMarch(vec3 p, vec3 ldir) {
    float t = 0.01;
    for (int i = 0; i < 64; ++i) {
        float d = sphereSDF(p + ldir * t, 1.0);
        if (d < 0.001) return 0.0;
        t += d;
        if (t > 100.0) break;
    }
    return 1.0;
}

void main() {
    // Coordonnées écran [-1,1]
    vec2 uv = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    // Calcul du rayon (caméra à camPos, regarde vers l'origine)
    vec3 ro = camPos;
    vec3 target = vec3(0.0);
    vec3 forward = normalize(target - camPos);
    vec3 right = normalize(cross(forward, vec3(0,1,0)));
    vec3 up = cross(right, forward);
    float fov = 45.0;
    float focal = 1.0 / tan(radians(fov) * 0.5);
    vec3 rd = normalize(forward * focal + uv.x * right + uv.y * up);

    vec3 pHit;
    float t = raymarch(ro, rd, pHit);
    if (t > 0.0) {
        // Normale
        vec3 n = normalize(pHit);
        // Ombre
        float shadow = shadowMarch(pHit + n * 0.01, lightDir);
        // Lumière
        float diff = max(dot(n, -normalize(lightDir)), 0.0) * shadow;
        vec3 color = mix(vec3(0.1,0.1,0.2), vec3(0.3,0.5,0.2), diff);
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(0.1,0.1,0.2,1.0); // fond
    }
}
