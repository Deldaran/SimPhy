#version 450 core
in vec3 teColor;
in vec3 teNormal;
in vec3 tePosition;
out vec4 FragColor;

uniform vec3 lightDir; // Ajouté pour l'éclairage

void main() {
    // Calcul de l'intensité lumineuse
    float intensity = max(dot(normalize(teNormal), -normalize(lightDir)), 0.0);
    // Ombre simple : si la face n'est pas éclairée, elle est noire
    vec3 color = teColor * intensity;
    FragColor = vec4(color, 1.0);
}
