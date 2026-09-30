#include "Renderer.h"

#include "Camera.h"
#include "ParticleSystem.h"
#include "Shader.h"

namespace {

constexpr int BLUR_PASSES = 10;

GLuint CreateColorTexture(int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

GLuint CreateFramebuffer(GLuint colorTex) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    return fbo;
}

// Triangle couvrant tout l'écran, généré dans fullscreen.vert à partir de gl_VertexID.
void DrawFullscreenTriangle() {
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

} // namespace

bool Renderer::Init(int width, int height) {
    particleProg = LoadGraphicsProgram("particles.vert", "particles.frag");
    blurProg = LoadGraphicsProgram("fullscreen.vert", "blur.frag");
    finalProg = LoadGraphicsProgram("fullscreen.vert", "final.frag");
    glGenVertexArrays(1, &emptyVAO);

    SetUniform(finalProg, "scene", 0);
    SetUniform(finalProg, "bloomBlur", 1);

    CreateTargets(width, height);
    return particleProg && blurProg && finalProg;
}

void Renderer::Destroy() {
    DestroyTargets();
    glDeleteVertexArrays(1, &emptyVAO);
    glDeleteProgram(particleProg);
    glDeleteProgram(blurProg);
    glDeleteProgram(finalProg);
}

void Renderer::CreateTargets(int width, int height) {
    targetWidth = width;
    targetHeight = height;
    hdrTex = CreateColorTexture(width, height);
    hdrFBO = CreateFramebuffer(hdrTex);
    for (int i = 0; i < 2; i++) {
        pingTex[i] = CreateColorTexture(width, height);
        pingFBO[i] = CreateFramebuffer(pingTex[i]);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DestroyTargets() {
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(1, &hdrTex);
    glDeleteFramebuffers(2, pingFBO);
    glDeleteTextures(2, pingTex);
}

void Renderer::Render(const ParticleSystem& particles, const Camera& camera, const RenderSettings& settings, int width, int height) {
    if (width != targetWidth || height != targetHeight) {
        DestroyTargets();
        CreateTargets(width, height);
    }

    glBindVertexArray(emptyVAO);

    // --- 1. Particules -> image HDR ---
    // Mélange additif : là où beaucoup d'étoiles se superposent, la luminosité s'accumule.
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    SetUniform(particleProg, "projection", camera.Projection(float(width) / float(height)));
    SetUniform(particleProg, "view", camera.View());
    SetUniform(particleProg, "perspective", camera.GetMode() == Camera::Mode::Free ? 1 : 0);
    SetUniform(particleProg, "speedColorMax", settings.speedColorMax);
    glUseProgram(particleProg);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particles.Buffer());
    glDrawArrays(GL_POINTS, 0, particles.Count());
    glDisable(GL_BLEND);

    // --- 2. Bloom : flou gaussien séparable, alterné horizontal / vertical ---
    bool horizontal = true;
    if (settings.bloom) {
        glUseProgram(blurProg);
        glActiveTexture(GL_TEXTURE0);
        for (int i = 0; i < BLUR_PASSES; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingFBO[horizontal]);
            SetUniform(blurProg, "horizontal", horizontal ? 1 : 0);
            glBindTexture(GL_TEXTURE_2D, i == 0 ? hdrTex : pingTex[!horizontal]);
            DrawFullscreenTriangle();
            horizontal = !horizontal;
        }
    }

    // --- 3. Composition finale : scène + bloom, tone mapping, gamma ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    SetUniform(finalProg, "bloom", settings.bloom ? 1 : 0);
    SetUniform(finalProg, "bloomIntensity", settings.bloomIntensity);
    SetUniform(finalProg, "exposure", settings.exposure);
    glUseProgram(finalProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingTex[!horizontal]);
    DrawFullscreenTriangle();
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}
