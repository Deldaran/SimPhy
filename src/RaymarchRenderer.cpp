#include "RaymarchRenderer.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>

RaymarchRenderer::RaymarchRenderer() {
    initQuad();
    loadShaders("src/raymarch.vert", "src/raymarch.frag");
    locCamPos = glGetUniformLocation(shaderProgram, "camPos");
    locCamRot = glGetUniformLocation(shaderProgram, "camRot");
    locSphereRadius = glGetUniformLocation(shaderProgram, "sphereRadius");
    locSphereCenter = glGetUniformLocation(shaderProgram, "sphereCenter");
    locLightDir = glGetUniformLocation(shaderProgram, "lightDir");
    locResolution = glGetUniformLocation(shaderProgram, "iResolution");
}

RaymarchRenderer::~RaymarchRenderer() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(shaderProgram);
}

void RaymarchRenderer::initQuad() {
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int RaymarchRenderer::compileShader(const std::string& src, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

void RaymarchRenderer::loadShaders(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    unsigned int vertShader = compileShader(vertSrc, GL_VERTEX_SHADER);
    unsigned int fragShader = compileShader(fragSrc, GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShader);
    glAttachShader(shaderProgram, fragShader);
    glBindAttribLocation(shaderProgram, 0, "position");
    glLinkProgram(shaderProgram);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

void RaymarchRenderer::render(const glm::vec3& camPos, const glm::mat3& camRot, float sphereRadius, const glm::vec3& sphereCenter, const glm::vec3& lightDir, int winWidth, int winHeight) {
    glUseProgram(shaderProgram);
    glUniform3fv(locCamPos, 1, &camPos.x);
    glUniformMatrix3fv(locCamRot, 1, GL_FALSE, &camRot[0][0]);
    glUniform1f(locSphereRadius, sphereRadius);
    glUniform3fv(locSphereCenter, 1, &sphereCenter.x);
    glUniform3fv(locLightDir, 1, &lightDir.x);
    glUniform2f(locResolution, (float)winWidth, (float)winHeight);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}
