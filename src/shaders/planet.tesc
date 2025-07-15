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

void main() {
    tc_out[gl_InvocationID].pos = vs_in[gl_InvocationID].pos;
    tc_out[gl_InvocationID].normal = vs_in[gl_InvocationID].normal;
    tc_out[gl_InvocationID].uv = vs_in[gl_InvocationID].uv;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 4.0;
        gl_TessLevelOuter[0] = 4.0;
        gl_TessLevelOuter[1] = 4.0;
        gl_TessLevelOuter[2] = 4.0;
    }
}