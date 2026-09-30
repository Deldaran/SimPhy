#version 430

in vec4 vColor;
out vec4 FragColor;

void main() {
    // Point rond aux bords adoucis
    vec2 coord = gl_PointCoord - vec2(0.5);
    float d = length(coord);
    if (d > 0.5) discard;
    float alpha = 1.0 - smoothstep(0.3, 0.5, d);
    FragColor = vec4(vColor.rgb * 1.5, vColor.a * alpha);
}
