#include "Scene.h"
#include <glad/glad.h>
#include <glad/glad.h>
#include <glm/glm.hpp>


#include <fstream>
#include <sstream>
#include <iostream>

GLuint loadShader(GLenum type, const char* path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string src = ss.str();
    GLuint shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}

GLuint createIcosphereProgram() {
    GLuint vs = loadShader(GL_VERTEX_SHADER, "src/icosphere.vert");
    GLuint tcs = loadShader(GL_TESS_CONTROL_SHADER, "src/icosphere.tesc");
    GLuint tes = loadShader(GL_TESS_EVALUATION_SHADER, "src/icosphere.tese");
    GLuint fs = loadShader(GL_FRAGMENT_SHADER, "src/icosphere.frag");
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, tcs);
    glAttachShader(prog, tes);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(tcs);
    glDeleteShader(tes);
    glDeleteShader(fs);
    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    return prog;
}

Scene::Scene()
    : camera(63710.0f, 45.0f, 30.0f), icosphere(6371.0f, 6) {
    icosphere.applyProceduralTerrain(0.4f, 200.0f);
    icosphere.initGLBuffers();
    icosphereProgram = createIcosphereProgram();
}


void Scene::update(float deltaTime) {
    // Camera update logic if needed
}

void Scene::render(bool wireframe) {
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glUseProgram(icosphereProgram);
    // Set uniforms
    GLint locModel = glGetUniformLocation(icosphereProgram, "uModel");
    GLint locView = glGetUniformLocation(icosphereProgram, "uView");
    GLint locProj = glGetUniformLocation(icosphereProgram, "uProj");
    GLint locLightDir = glGetUniformLocation(icosphereProgram, "lightDir");
    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    glUniform3fv(locLightDir, 1, &lightDir.x);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 10.0f, 200000.0f);
    glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, &proj[0][0]);
    icosphere.draw();
    glUseProgram(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


Camera& Scene::getCamera() { return camera; }
Icosphere& Scene::getIcosphere() { return icosphere; }
void Scene::regenerateIcosphere(float radius, int subdivisions) {
    icosphere.cleanupGLBuffers();
    icosphere = Icosphere(radius, subdivisions);
    icosphere.applyProceduralTerrain(0.4f, 20.0f);
    icosphere.initGLBuffers();
    camera = Camera(radius * 1.5f, camera.getYaw(), camera.getPitch());
}
