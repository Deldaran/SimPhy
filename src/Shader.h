#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Chargement des shaders depuis le dossier shaders/ (voir SHADER_DIR dans CMakeLists.txt).
// Renvoient 0 en cas d'erreur, après avoir affiché le journal de compilation.
GLuint LoadComputeProgram(const std::string& computeFile);
GLuint LoadGraphicsProgram(const std::string& vertexFile, const std::string& fragmentFile);

// glProgramUniform* écrit directement dans le programme, sans avoir à le lier avant.
inline void SetUniform(GLuint prog, const char* name, float v) { glProgramUniform1f(prog, glGetUniformLocation(prog, name), v); }
inline void SetUniform(GLuint prog, const char* name, int v) { glProgramUniform1i(prog, glGetUniformLocation(prog, name), v); }
inline void SetUniform(GLuint prog, const char* name, unsigned v) { glProgramUniform1ui(prog, glGetUniformLocation(prog, name), v); }
inline void SetUniform(GLuint prog, const char* name, const glm::mat4& m) {
    glProgramUniformMatrix4fv(prog, glGetUniformLocation(prog, name), 1, GL_FALSE, &m[0][0]);
}
