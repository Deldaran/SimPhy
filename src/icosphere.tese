#version 450 core
layout(triangles, equal_spacing, cw) in;

in vec3 vNormal[];
in vec3 vColor[];
in vec3 vPosition[];
out vec3 teNormal;
out vec3 teColor;
out vec3 tePosition;

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 pos = gl_TessCoord.x * p0 + gl_TessCoord.y * p1 + gl_TessCoord.z * p2;
    teNormal = normalize(gl_TessCoord.x * vNormal[0] + gl_TessCoord.y * vNormal[1] + gl_TessCoord.z * vNormal[2]);
    teColor = gl_TessCoord.x * vColor[0] + gl_TessCoord.y * vColor[1] + gl_TessCoord.z * vColor[2];
    tePosition = gl_TessCoord.x * vPosition[0] + gl_TessCoord.y * vPosition[1] + gl_TessCoord.z * vPosition[2];
    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;
}
