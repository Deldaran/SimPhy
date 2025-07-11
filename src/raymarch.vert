#version 130
// Vertex shader for fullscreen quad
in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
