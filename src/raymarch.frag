#version 130
// Raymarching fragment shader for a sphere with soft shadows and realistic lighting
out vec4 fragColor;

uniform vec3 camPos;
uniform mat3 camRot;
uniform vec3 lightDir;
uniform float sphereRadius;
uniform vec3 sphereCenter;
uniform vec2 iResolution;

float sphereSDF(vec3 p) {
    return length(p - sphereCenter) - sphereRadius;
}

float softShadow(vec3 ro, vec3 rd, float mint, float maxt, float k) {
    float res = 1.0;
    float t = mint;
    for (int i = 0; i < 64; ++i) {
        float h = sphereSDF(ro + rd * t);
        if (h < 0.001) return 0.0;
        res = min(res, k * h / t);
        t += clamp(h, 0.01, 0.2);
        if (t > maxt) break;
    }
    return clamp(res, 0.0, 1.0);
}

vec3 getNormal(vec3 p) {
    float eps = 0.001;
    vec3 n;
    n.x = sphereSDF(p + vec3(eps,0,0)) - sphereSDF(p - vec3(eps,0,0));
    n.y = sphereSDF(p + vec3(0,eps,0)) - sphereSDF(p - vec3(0,eps,0));
    n.z = sphereSDF(p + vec3(0,0,eps)) - sphereSDF(p - vec3(0,0,eps));
    return normalize(n);
}

void main() {
    // Ray setup
    vec2 uv = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    vec3 rd = normalize(camRot * vec3(uv, -1.5));
    vec3 ro = camPos;

    // Raymarching
    float t = 0.0;
    float d = 0.0;
    bool hit = false;
    for (int i = 0; i < 128; ++i) {
        vec3 p = ro + rd * t;
        d = sphereSDF(p);
        if (d < 0.001) { hit = true; break; }
        t += d;
        if (t > 100.0) break;
    }

    if (hit) {
        vec3 p = ro + rd * t;
        vec3 n = getNormal(p);
        float diff = max(dot(n, -lightDir), 0.0);
        float shadow = softShadow(p + n * 0.01, -lightDir, 0.01, 5.0, 32.0);
        float ambient = 0.15;
        vec3 color = mix(vec3(0.1,0.2,0.5), vec3(0.3,0.7,0.3), diff * shadow);
        fragColor = vec4(color * (ambient + diff * shadow), 1.0);
    } else {
        fragColor = vec4(0.1, 0.1, 0.2, 1.0);
    }
}
