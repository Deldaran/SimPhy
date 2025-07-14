#version 450 core
layout(vertices = 3) out;


in vec3 vNormal[];
in vec3 vColor[];
in vec3 vPosition[];
out vec3 tcNormal[];
out vec3 tcColor[];
out vec3 tcPosition[];

// Rayon de la planète (1 unité = 1 km)
const float planetRadius = 6371000.0; // 1 unité = 1m
void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tcNormal[gl_InvocationID] = vNormal[gl_InvocationID];
    tcColor[gl_InvocationID] = vColor[gl_InvocationID];
    tcPosition[gl_InvocationID] = vPosition[gl_InvocationID] * planetRadius;
    if (gl_InvocationID == 0) {
        // Tessellation dynamique selon la distance caméra
        float camDist = length(tcPosition[0]);
        float tessLevel = 8.0;
        if (camDist < planetRadius * 1.05) tessLevel = 64.0;
        else if (camDist < planetRadius * 1.2) tessLevel = 32.0;
        else if (camDist < planetRadius * 2.0) tessLevel = 16.0;
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;
        gl_TessLevelInner[0] = tessLevel;
    }
}
