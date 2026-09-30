#version 430
// Un seul triangle qui déborde de l'écran et le couvre entièrement, sans vertex buffer :
// sommets (0,0), (2,0), (0,2) en coordonnées de texture.

out vec2 TexCoords;

void main() {
    vec2 uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    TexCoords = uv;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
