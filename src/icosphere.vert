#version 450 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

out vec3 vNormal;
out vec3 vColor;
out vec3 vPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vNormal = mat3(uModel) * inNormal;
    vColor = inColor;
    vPosition = vec3(uModel * vec4(inPosition, 1.0));
    gl_Position = uProj * uView * uModel * vec4(inPosition, 1.0);
}
