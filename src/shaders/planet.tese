#version 450 core
layout(triangles, equal_spacing, ccw) in;

in gl_PerVertex {
    vec4 gl_Position;
} gl_in[];
out gl_PerVertex {
    vec4 gl_Position;
};

in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} tc_in[];
out VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} te_out;

void main() {
    te_out.pos = tc_in[0].pos * gl_TessCoord.x + tc_in[1].pos * gl_TessCoord.y + tc_in[2].pos * gl_TessCoord.z;
    te_out.normal = normalize(tc_in[0].normal * gl_TessCoord.x + tc_in[1].normal * gl_TessCoord.y + tc_in[2].normal * gl_TessCoord.z);
    te_out.uv = tc_in[0].uv * gl_TessCoord.x + tc_in[1].uv * gl_TessCoord.y + tc_in[2].uv * gl_TessCoord.z;
    gl_Position = gl_in[0].gl_Position * gl_TessCoord.x +
                  gl_in[1].gl_Position * gl_TessCoord.y +
                  gl_in[2].gl_Position * gl_TessCoord.z;
}