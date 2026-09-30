#version 430

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;
uniform float bloomIntensity;

void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    if (bloom)
        color += texture(bloomBlur, TexCoords).rgb * bloomIntensity;

    // Tone mapping exponentiel : ramène les luminosités HDR (0 à ∞) dans [0, 1].
    color = vec3(1.0) - exp(-color * exposure);
    // Correction gamma (écran sRGB)
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
