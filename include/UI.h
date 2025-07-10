#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>
#include <functional>

class UI {
private:
    bool m_initialized;
    bool m_showTimeControls;
    
    // Callback pour changer le timeScale
    std::function<void(float)> m_timeScaleChangeCallback;

public:
    UI();
    ~UI();
    
    bool initialize(GLFWwindow* window);
    void cleanup();
    
    void beginFrame();
    void render();
    
    // Interface pour afficher le menu de contrôle du temps
    void showTimeControlsWindow(float& timeScale);
    
    // Affichage/masquage du menu de contrôle du temps
    void setShowTimeControls(bool show) { m_showTimeControls = show; }
    bool isShowingTimeControls() const { return m_showTimeControls; }
    void toggleTimeControls() { m_showTimeControls = !m_showTimeControls; }
    
    // Définit le callback de changement de timeScale
    void setTimeScaleChangeCallback(std::function<void(float)> callback) {
        m_timeScaleChangeCallback = std::move(callback);
    }
};
