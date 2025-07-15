


#version 450 core
out vec4 FragColor;

in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} te_out;

// Relief procédural simple
float fbm(vec3 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; ++i) {
        v += a * sin(p.x * 2.0 + float(i) * 1.7) * sin(p.y * 2.0 + float(i) * 1.3) * sin(p.z * 2.0 + float(i) * 1.5);
        p *= 1.8;
        a *= 0.5;
    }
    return v;
}

void main() {
    // Position et normale interpolées
    vec3 pos = te_out.pos;
    vec3 normal = normalize(te_out.normal);

    // Ajout d'un relief procédural
    float relief = fbm(pos * 0.001) * 50.0;
    pos += normal * relief;
    normal = normalize(normal + vec3(
        fbm((pos + vec3(0.1,0,0)) * 0.001) - fbm((pos - vec3(0.1,0,0)) * 0.001),
        fbm((pos + vec3(0,0.1,0)) * 0.001) - fbm((pos - vec3(0,0.1,0)) * 0.001),
        fbm((pos + vec3(0,0,0.1)) * 0.001) - fbm((pos - vec3(0,0,0.1)) * 0.001)
    ));

    // Lumière
    vec3 lightDir = normalize(vec3(10, 10, 10));
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(reflect(-lightDir, normal), normalize(pos)), 0.0), 32.0);
    vec3 baseColor = vec3(0.2, 0.5, 0.8);
    FragColor = vec4(baseColor * diff + vec3(0.5) * spec, 1.0);
}
