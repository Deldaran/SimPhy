#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    // Approximation normale : direction du sommet depuis le centre du vaisseau
    Normal = normalize(aPos);
    gl_Position = projection * view * worldPos;
}
