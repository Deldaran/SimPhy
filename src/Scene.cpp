#include "Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <fstream>
#include <sstream>
#include <iostream>

GLuint loadShader(GLenum type, const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Shader file not found: " << path << std::endl;
        std::ofstream errorFile("shader_errors.txt", std::ios::app);
        errorFile << "Shader file not found: " << path << std::endl;
        errorFile.close();
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string src = ss.str();
    std::cout << "Loading shader: " << path << std::endl;
    GLuint shader = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(shader, 1, &csrc, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error in " << path << ":\n" << log << std::endl;
        std::ofstream errorFile("shader_errors.txt", std::ios::app);
        errorFile << "Shader compile error in " << path << ":\n" << log << std::endl;
        errorFile.close();
    }
    return shader;
}

GLuint createIcosphereProgram() {
    GLuint vs = loadShader(GL_VERTEX_SHADER, "c:/Galaxy CPU/src/icosphere.vert");
    GLuint tcs = loadShader(GL_TESS_CONTROL_SHADER, "c:/Galaxy CPU/src/icosphere.tesc");
    GLuint tes = loadShader(GL_TESS_EVALUATION_SHADER, "c:/Galaxy CPU/src/icosphere.tese");
    GLuint fs = loadShader(GL_FRAGMENT_SHADER, "c:/Galaxy CPU/src/icosphere.frag");
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
        std::cerr << "Program link error:\n" << log << std::endl;
        std::ofstream errorFile("shader_errors.txt", std::ios::app);
        errorFile << "Program link error:\n" << log << std::endl;
        errorFile.close();
    }
    return prog;
}


// Rayon planète
const float PLANET_RADIUS = 6371000.0f; // 1 unité = 1m
const float CAMERA_START_RADIUS = PLANET_RADIUS * 3.0f; // Caméra à 3x le rayon

Scene::Scene()
    : camera(CAMERA_START_RADIUS, 45.0f, 30.0f),
      icosphere(glm::vec3(0.0f, 0.0f, 0.0f), PLANET_RADIUS, 6)
{
    icosphere.applyProceduralTerrain(0.4f, 2000.0f);
    icosphere.initGLBuffers();
    icosphereProgram = createIcosphereProgram();
    // Ajoute la planète comme objet sélectionnable
    objects.push_back({ icosphere.getCenter(), "Planète" });
    selectedIndex = 0;
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
    // Lumière type Soleil à 150 millions de km sur l'axe Z (1 unité = 1km)
    glm::vec3 sunPos = glm::vec3(0.0f, 0.0f, 150000000000.0f); // 150 000 000 km -> 150 000 000 000 m
    glm::vec3 planetPos = icosphere.getCenter();
    glm::vec3 lightDir = glm::normalize(sunPos - planetPos);
    glUniform3fv(locLightDir, 1, &lightDir.x);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 10.0f, 200000000000000000.0f);
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
    icosphere = Icosphere(glm::vec3(0.0f), radius, subdivisions);
    icosphere.applyProceduralTerrain(0.4f, 2000.0f);
    icosphere.initGLBuffers();
    camera = Camera(radius * 3.0f, camera.getYaw(), camera.getPitch());
    // Met à jour la liste d'objets sélectionnables
    objects.clear();
    glm::vec3 planetCenter = icosphere.getVertices().size() > 0 ? icosphere.getVertices()[0].position * 0.0f : glm::vec3(0.0f);
    objects.push_back({ planetCenter, "Planète" });
    selectedIndex = 0;
}
