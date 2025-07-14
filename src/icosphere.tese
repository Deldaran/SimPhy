#version 450 core
layout(triangles, equal_spacing, cw) in;

in vec3 tcNormal[];
in vec3 tcColor[];
in vec3 tcPosition[];
out vec3 teNormal;
out vec3 teColor;
out vec3 tePosition;


// Rayon de la planète (1 unité = 1 km)
const float planetRadius = 6371000.0; // 1 unité = 1m
void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz * planetRadius;
    vec3 p1 = gl_in[1].gl_Position.xyz * planetRadius;
    vec3 p2 = gl_in[2].gl_Position.xyz * planetRadius;
    vec3 pos = gl_TessCoord.x * p0 + gl_TessCoord.y * p1 + gl_TessCoord.z * p2;
    teNormal = normalize(gl_TessCoord.x * tcNormal[0] + gl_TessCoord.y * tcNormal[1] + gl_TessCoord.z * tcNormal[2]);
    teColor = gl_TessCoord.x * tcColor[0] + gl_TessCoord.y * tcColor[1] + gl_TessCoord.z * tcColor[2];
    tePosition = gl_TessCoord.x * tcPosition[0] + gl_TessCoord.y * tcPosition[1] + gl_TessCoord.z * tcPosition[2];
    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
}
