#version 450 core
out vec4 FragColor;

uniform vec3 sunColor;

void main() {
    FragColor = vec4(sunColor, 1.0);
}
