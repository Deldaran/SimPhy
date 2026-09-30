#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Camera.h"
#include "InitialConditions.h"
#include "ParticleSystem.h"
#include "Renderer.h"
#include "Units.h"

#include <cmath>
#include <fstream>
#include <iostream>

#ifdef _WIN32
// Sur les portables à double GPU (NVIDIA Optimus, AMD PowerXpress), demande le GPU dédié.
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace {

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int PARTICLE_COUNTS[] = { 1 << 14, 1 << 16, 1 << 18, 1 << 20 };
const char* PARTICLE_COUNT_LABELS[] = { "16 384", "65 536", "262 144", "1 048 576" };

void APIENTRY OnGLDebugMessage(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    std::cerr << (type == GL_DEBUG_TYPE_ERROR ? "[GL ERREUR] " : "[GL] ") << message << std::endl;
}

// Police Segoe UI (Windows) avec l'alphabet grec, pour afficher ε, σ, ω dans l'interface.
void LoadFont() {
    const char* path = "C:/Windows/Fonts/segoeui.ttf";
    if (!std::ifstream(path)) return; // sinon, police par défaut d'ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(path, 17.0f, nullptr, io.Fonts->GetGlyphRangesGreek());
}

struct AppState {
    DiskParams disk;
    PhysicsParams physics;
    RenderSettings render;
    bool paused = false;
    double simTime = 0.0; // Myr
};

void ResetSimulation(AppState& state, ParticleSystem& particles) {
    particles.Upload(MakeExponentialDisk(state.disk, state.physics));
    state.simTime = 0.0;
}

void DrawUI(GLFWwindow* window, AppState& state, ParticleSystem& particles, Camera& camera) {
    if (camera.GetMode() == Camera::Mode::Free) {
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::Begin("##free", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        ImGui::Text("Vol libre : souris + WASD, Maj = rapide, Échap = sortir");
        ImGui::Text("t = %.1f Myr", state.simTime);
        ImGui::End();
        return;
    }

    ImGui::Begin("Simulation");
    ImGui::Text("%u particules  |  %.0f FPS", particles.Count(), ImGui::GetIO().Framerate);
    ImGui::Text("Temps simulé : %.1f Myr (%.3f Gyr)", state.simTime, state.simTime / 1000.0);

    if (ImGui::Button(state.paused ? "Reprendre" : "Pause")) state.paused = !state.paused;
    ImGui::SameLine();
    if (ImGui::Button("Réinitialiser")) ResetSimulation(state, particles);
    ImGui::SameLine();
    if (ImGui::Button("Vol libre 3D")) camera.SetMode(window, Camera::Mode::Free);
    ImGui::SameLine();
    if (ImGui::Button("Recentrer")) camera.ResetView();

    if (ImGui::CollapsingHeader("Physique", ImGuiTreeNodeFlags_DefaultOpen)) {
        PhysicsParams& ph = state.physics;
        ImGui::SliderFloat("Masse centrale (1e10 Msol)", &ph.centralMass, 0.0f, 50.0f);
        ImGui::SliderFloat("Adoucissement ε (kpc)", &ph.softening, 0.05f, 5.0f);
        ImGui::SliderFloat("Pas de temps dt (Myr)", &ph.dt, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderInt("Pas par image", &ph.stepsPerFrame, 1, 20);

        // Grandeurs de référence pour comprendre les réglages
        const float r0 = 8.0f;
        float vc = CircularVelocity(r0, ph);
        ImGui::Text("À R = 8 kpc : v_c = %.0f km/s, période = %.0f Myr",
                    Units::ToKms(vc), vc > 0.0f ? 2.0f * 3.14159265f * r0 / vc : 0.0f);

        // Au centre, une particule oscille à la pulsation ω = sqrt(G M / ε³).
        // Le leapfrog est stable si ω·dt < 2, et précis si ω·dt < ~0,1.
        float omegaDt = ph.dt * std::sqrt(Units::G * ph.centralMass / (ph.softening * ph.softening * ph.softening));
        ImVec4 color = omegaDt < 0.1f ? ImVec4(0.4f, 1, 0.4f, 1) : omegaDt < 1.0f ? ImVec4(1, 0.8f, 0.2f, 1) : ImVec4(1, 0.3f, 0.3f, 1);
        ImGui::TextColored(color, "Précision au centre : ω·dt = %.3f (stable < 2, précis < 0,1)", omegaDt);
    }

    if (ImGui::CollapsingHeader("Conditions initiales (appliquées à la réinitialisation)", ImGuiTreeNodeFlags_DefaultOpen)) {
        DiskParams& d = state.disk;
        int countIndex = 0;
        for (int i = 0; i < 4; i++)
            if (PARTICLE_COUNTS[i] == d.count) countIndex = i;
        if (ImGui::Combo("Particules", &countIndex, PARTICLE_COUNT_LABELS, 4)) d.count = PARTICLE_COUNTS[countIndex];

        ImGui::SliderFloat("Longueur d'échelle R_d (kpc)", &d.scaleLength, 0.5f, 10.0f);
        ImGui::SliderFloat("Rayon max (kpc)", &d.maxRadius, 5.0f, 50.0f);
        ImGui::SliderFloat("Épaisseur σ_z (kpc)", &d.scaleHeight, 0.0f, 2.0f);
        float sigmaKms = Units::ToKms(d.velocityDispersion);
        if (ImGui::SliderFloat("Dispersion σ_v (km/s)", &sigmaKms, 0.0f, 100.0f)) d.velocityDispersion = Units::FromKms(sigmaKms);
        int seed = static_cast<int>(d.seed);
        if (ImGui::InputInt("Graine aléatoire", &seed)) d.seed = static_cast<uint32_t>(seed);
    }

    if (ImGui::CollapsingHeader("Rendu")) {
        RenderSettings& r = state.render;
        ImGui::Checkbox("Bloom", &r.bloom);
        ImGui::SliderFloat("Intensité bloom", &r.bloomIntensity, 0.0f, 2.0f);
        ImGui::SliderFloat("Exposition", &r.exposure, 0.1f, 5.0f);
        float speedKms = Units::ToKms(r.speedColorMax);
        if (ImGui::SliderFloat("Vitesse « rouge » (km/s)", &speedKms, 50.0f, 1000.0f)) r.speedColorMax = Units::FromKms(speedKms);
    }
    ImGui::End();
}

} // namespace

int main() {
    if (!glfwInit()) return -1;

    // OpenGL 4.3 : premier niveau avec les compute shaders et les SSBO.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SimPhy - Galaxie", nullptr, nullptr);
    if (!window) {
        std::cerr << "Impossible de créer un contexte OpenGL 4.3 (pilote graphique à jour ?)" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Échec du chargement d'OpenGL (glad)" << std::endl;
        return -1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " - " << glGetString(GL_RENDERER) << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OnGLDebugMessage, nullptr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    LoadFont();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    AppState state;
    ParticleSystem particles;
    Renderer renderer;
    Camera camera;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    if (!particles.Init() || !renderer.Init(fbWidth, fbHeight)) {
        std::cerr << "Échec de l'initialisation des shaders" << std::endl;
        return -1;
    }
    ResetSimulation(state, particles);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float realDt = static_cast<float>(now - lastTime);
        lastTime = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        camera.Update(window, realDt);
        DrawUI(window, state, particles, camera);

        // Physique à pas fixe : le résultat ne dépend pas des FPS,
        // seule la vitesse de défilement à l'écran en dépend.
        if (!state.paused) {
            for (int i = 0; i < state.physics.stepsPerFrame; i++) {
                particles.Step(state.physics);
                state.simTime += state.physics.dt;
            }
        }

        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        if (fbWidth > 0 && fbHeight > 0) // fenêtre réduite : rien à dessiner
            renderer.Render(particles, camera, state.render, fbWidth, fbHeight);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    renderer.Destroy();
    particles.Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
