#version 430
// Chaque sommet est une particule : on la lit directement dans le SSBO via gl_VertexID.

struct Particle { vec4 pos; vec4 vel; vec4 acc; }; // identique à Particle (ParticleSystem.h)
layout(std430, binding = 0) readonly buffer Particles { Particle particles[]; };

uniform mat4 projection;
uniform mat4 view;
uniform bool perspective;
uniform float speedColorMax; // kpc/Myr

out vec4 vColor;

void main() {
    Particle p = particles[gl_VertexID];
    vec4 viewPos = view * vec4(p.pos.xyz, 1.0);
    gl_Position = projection * viewPos;

    // En perspective, les particules proches sont plus grosses.
    gl_PointSize = perspective ? clamp(40.0 / length(viewPos.xyz), 1.0, 6.0) : 2.0;

    // Couleur selon la vitesse : bleu = lent, rouge = rapide.
    float t = clamp(length(p.vel.xyz) / speedColorMax, 0.0, 1.0);
    vColor = mix(vec4(0.0, 0.5, 1.0, 1.0), vec4(1.0, 0.2, 0.1, 1.0), t);

    // Les particules loin du plan du disque (|z| > 0,3 kpc) sont atténuées.
    float fade = 1.0 - smoothstep(0.3, 3.0, abs(p.pos.z));
    vColor.a *= 0.3 + 0.7 * fade;
}
