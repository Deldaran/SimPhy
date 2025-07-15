#version 450 core
out gl_PerVertex {
    vec4 gl_Position;
};
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vs_out.pos = vec3(model * vec4(aPos, 1.0));
    vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.uv = aUV;
    gl_Position = projection * view * vec4(vs_out.pos, 1.0);
}
