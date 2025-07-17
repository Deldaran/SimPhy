#version 450 core
layout(vertices = 3) out;


in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];
out gl_PerVertex {
    vec4 gl_Position;
} gl_out[];

in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} vs_in[];
out VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} tc_out[];

uniform vec3 cameraPos;

void main() {
    tc_out[gl_InvocationID].pos = vs_in[gl_InvocationID].pos;
    tc_out[gl_InvocationID].normal = vs_in[gl_InvocationID].normal;
    tc_out[gl_InvocationID].uv = vs_in[gl_InvocationID].uv;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0) {
        // Calcul de la distance moyenne du triangle à la caméra
        float d0 = distance(vs_in[0].pos, cameraPos);
        float d1 = distance(vs_in[1].pos, cameraPos);
        float d2 = distance(vs_in[2].pos, cameraPos);
        float avgDist = (d0 + d1 + d2) / 3.0;
        // LOD dynamique : plus proche = plus de subdivisions
        float tessLevel = clamp(32.0 / (avgDist * 0.01), 2.0, 8.0); // Ajuste selon ta scène
        gl_TessLevelInner[0] = tessLevel;
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;
    }
}