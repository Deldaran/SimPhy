#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Impossible d'ouvrir le shader : " << path << std::endl;
        return false;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

GLuint CompileShader(GLenum type, const std::string& file) {
    std::string source;
    if (!ReadFile(std::string(SHADER_DIR) + file, source)) return 0;

    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len + 1);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::cerr << "Erreur de compilation (" << file << ") :\n" << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint LinkProgram(const GLuint* shaders, int count, const std::string& name) {
    GLuint prog = glCreateProgram();
    for (int i = 0; i < count; i++) glAttachShader(prog, shaders[i]);
    glLinkProgram(prog);
    for (int i = 0; i < count; i++) glDeleteShader(shaders[i]);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len + 1);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Erreur d'édition de liens (" << name << ") :\n" << log.data() << std::endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

} // namespace

GLuint LoadComputeProgram(const std::string& computeFile) {
    GLuint cs = CompileShader(GL_COMPUTE_SHADER, computeFile);
    if (!cs) return 0;
    return LinkProgram(&cs, 1, computeFile);
}

GLuint LoadGraphicsProgram(const std::string& vertexFile, const std::string& fragmentFile) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexFile);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentFile);
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }
    GLuint shaders[] = { vs, fs };
    return LinkProgram(shaders, 2, vertexFile + " + " + fragmentFile);
}
