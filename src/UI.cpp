#include "UI.h"
#include <iostream>

UI::UI() : m_initialized(false), m_showTimeControls(false) {
}

UI::~UI() {
    cleanup();
}

bool UI::initialize(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Configure ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    m_initialized = true;
    std::cout << "UI system initialized" << std::endl;
    
    return true;
}

void UI::cleanup() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void UI::beginFrame() {
    if (!m_initialized) return;
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::render() {
    if (!m_initialized) return;
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::showTimeControlsWindow(float& timeScale) {
    if (!m_initialized || !m_showTimeControls) return;
    
    // Commencer la fenêtre ImGui
    ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Contrôles de Temps", &m_showTimeControls)) {
        // Afficher la vitesse actuelle
        ImGui::Text("Vitesse actuelle: x%.2f", timeScale);
        
        // Calculer et afficher la durée d'une année terrestre avec cette échelle de temps
        const float earthYearInRealSeconds = 31557600.0f / timeScale; // 365.25 * 24 * 60 * 60 secondes
        float days, hours, minutes, seconds;
        seconds = fmodf(earthYearInRealSeconds, 60.0f);
        minutes = fmodf(earthYearInRealSeconds / 60.0f, 60.0f);
        hours = fmodf(earthYearInRealSeconds / 3600.0f, 24.0f);
        days = earthYearInRealSeconds / 86400.0f;
        
        if (days >= 1.0f) {
            ImGui::Text("Une année terrestre = %.0f jours %.0f h %.0f min", days, hours, minutes);
        } else if (hours >= 1.0f) {
            ImGui::Text("Une année terrestre = %.0f h %.0f min %.0f sec", hours, minutes, seconds);
        } else {
            ImGui::Text("Une année terrestre = %.0f min %.0f sec", minutes, seconds);
        }
        
        ImGui::Separator();
        
        // Slider pour ajuster la vitesse
        float logTimeScale = log2f(timeScale); // Convertir en échelle logarithmique
        if (ImGui::SliderFloat("Échelle de temps", &logTimeScale, -3.0f, 6.0f, "x%.2f")) {
            // Convertir de l'échelle logarithmique en échelle linéaire
            float newTimeScale = powf(2.0f, logTimeScale);
            timeScale = newTimeScale;
            
            // Appeler le callback si défini
            if (m_timeScaleChangeCallback) {
                m_timeScaleChangeCallback(timeScale);
            }
        }
        
        // Boutons pour des valeurs prédéfinies
        ImGui::Text("Présélections:");
        if (ImGui::Button("x0.25")) {
            timeScale = 0.25f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x0.5")) {
            timeScale = 0.5f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x1.0")) {
            timeScale = 1.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x2.0")) {
            timeScale = 2.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x4.0")) {
            timeScale = 4.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        
        // Boutons pour vitesses élevées (simulation rapide)
        if (ImGui::Button("x100")) {
            timeScale = 100.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x1000")) {
            timeScale = 1000.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("x10000")) {
            timeScale = 10000.0f;
            if (m_timeScaleChangeCallback) m_timeScaleChangeCallback(timeScale);
        }
        
        ImGui::Separator();
        ImGui::Text("Touches: T pour afficher/masquer ce menu");
        ImGui::Text("         + pour accélérer (x2)");
        ImGui::Text("         - pour ralentir (÷2)");
        ImGui::Text("         0 pour réinitialiser (x1)");
    }
    ImGui::End();
}
