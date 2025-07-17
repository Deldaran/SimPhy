#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>

class SunSphere {
public:
    GLuint vao = 0, vbo = 0;
    std::vector<glm::vec3> vertices;
    float radius;
    glm::vec3 center;
    SunSphere(glm::vec3 c, float r);
    void initGLBuffers();
    void draw();
    void cleanupGLBuffers();
};
