#include "SunSphere.h"
#include <cmath>

SunSphere::SunSphere(glm::vec3 c, float r) : center(c), radius(r) {
    // Simple UV sphere (low poly)
    const float PI = 3.14159265358979323846f;
    int stacks = 12, slices = 24;
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * float(j) / float(slices);
            float x = r * sin(phi) * cos(theta);
            float y = r * cos(phi);
            float z = r * sin(phi) * sin(theta);
            vertices.push_back(c + glm::vec3(x, y, z));
        }
    }
}

void SunSphere::initGLBuffers() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);
}

void SunSphere::draw() {
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)vertices.size());
    glBindVertexArray(0);
}

void SunSphere::cleanupGLBuffers() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}
