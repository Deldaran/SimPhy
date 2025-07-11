#pragma once
#include <glm/glm.hpp>
#include <string>

class RaymarchRenderer {
public:
    RaymarchRenderer();
    ~RaymarchRenderer();
    void render(const glm::vec3& camPos, const glm::mat3& camRot, float sphereRadius, const glm::vec3& sphereCenter, const glm::vec3& lightDir, int winWidth, int winHeight);
private:
    unsigned int quadVAO, quadVBO;
    unsigned int shaderProgram;
    int locCamPos, locCamRot, locSphereRadius, locSphereCenter, locLightDir, locResolution;
    void initQuad();
    void loadShaders(const std::string& vertPath, const std::string& fragPath);
    unsigned int compileShader(const std::string& src, unsigned int type);
};
