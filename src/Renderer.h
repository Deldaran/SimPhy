#pragma once

#include <glad/glad.h>

class Camera;
class ParticleSystem;

struct RenderSettings {
    bool bloom = true;
    float bloomIntensity = 0.8f;
    float exposure = 0.8f;
    float speedColorMax = 0.4f; // vitesse (kpc/Myr) affichée en rouge ; 0,4 ≈ 390 km/s
};

// Rendu des particules dans une image HDR, puis bloom (flou gaussien) et tone mapping.
class Renderer {
public:
    bool Init(int width, int height);
    void Destroy();
    void Render(const ParticleSystem& particles, const Camera& camera, const RenderSettings& settings, int width, int height);

private:
    void CreateTargets(int width, int height);
    void DestroyTargets();

    GLuint particleProg = 0;
    GLuint blurProg = 0;
    GLuint finalProg = 0;
    GLuint emptyVAO = 0; // le profil core exige un VAO lié, même si les sommets viennent du SSBO

    GLuint hdrFBO = 0, hdrTex = 0;
    GLuint pingFBO[2] = {}, pingTex[2] = {};
    int targetWidth = 0, targetHeight = 0;
};
