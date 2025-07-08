#pragma once
#include <windows.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

class Camera;
class Planet;

class DebugWindow {
private:
    HWND m_hwnd;
    std::thread m_windowThread;
    std::atomic<bool> m_isOpen;
    std::atomic<bool> m_shouldClose;
    std::mutex m_dataMutex;
    
    // Debug info (thread-safe)
    glm::vec3 m_cameraPosition;
    float m_nearestDistance;
    int m_nearestPlanetIndex;
    float m_speedMultiplier;
    std::vector<float> m_planetDistances;
    std::vector<std::string> m_planetNames;
    
    // FPS tracking
    float m_currentFPS;
    float m_averageFPS;
    int m_frameCount;
    std::chrono::steady_clock::time_point m_lastFPSUpdate;
    
    // Window procedures
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void windowThreadFunction();
    void paintWindow(HDC hdc);

public:
    DebugWindow();
    ~DebugWindow();
    
    bool initialize();
    void update(Camera* camera, const std::vector<std::unique_ptr<Planet>>& planets);
    void updateFPS(float deltaTime);
    void cleanup();
    
    bool shouldClose() const { return m_shouldClose.load(); }
    bool isOpen() const { return m_isOpen.load(); }
    void close() { m_shouldClose.store(true); }
};
