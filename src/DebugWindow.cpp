#include "DebugWindow.h"
#include "Camera.h"
#include "Planet.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <chrono>
#include <thread>

DebugWindow::DebugWindow() : m_hwnd(nullptr), m_isOpen(false), m_shouldClose(false),
                             m_nearestDistance(0.0f), m_nearestPlanetIndex(-1), 
                             m_speedMultiplier(1.0f) {
    // Initialize planet names
    m_planetNames = {
        "Soleil", "Mercure", "Venus", "Terre", "Mars", 
        "Jupiter", "Saturne", "Uranus", "Neptune"
    };
}

DebugWindow::~DebugWindow() {
    cleanup();
}

bool DebugWindow::initialize() {
    m_shouldClose.store(false);
    
    // Create window in separate thread
    m_windowThread = std::thread(&DebugWindow::windowThreadFunction, this);
    
    // Wait a bit for window creation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return m_isOpen.load();
}

void DebugWindow::windowThreadFunction() {
    // Register window class
    const char CLASS_NAME[] = "GalaxyDebugWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    RegisterClass(&wc);
    
    // Create window
    m_hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        "Galaxy Debug Info",            // Window text
        WS_OVERLAPPEDWINDOW,            // Window style
        
        // Size and position
        1400, 100, 450, 600,
        
        nullptr,       // Parent window
        nullptr,       // Menu
        GetModuleHandle(nullptr),  // Instance handle
        this           // Additional application data
    );
    
    if (m_hwnd == nullptr) {
        return;
    }
    
    // Show window
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    m_isOpen.store(true);
    
    // Message loop
    MSG msg = {};
    while (!m_shouldClose.load()) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_shouldClose.store(true);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // Trigger repaint every 100ms
        InvalidateRect(m_hwnd, nullptr, TRUE);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    m_isOpen.store(false);
}

LRESULT CALLBACK DebugWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DebugWindow* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (DebugWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (DebugWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        switch (uMsg) {
        case WM_DESTROY:
            pThis->m_shouldClose.store(true);
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                pThis->paintWindow(hdc);
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void DebugWindow::paintWindow(HDC hdc) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Set up drawing
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    
    // Clear background
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    
    // Set text properties
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    // Create font
    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas"
    );
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    int y = 20;
    const int lineHeight = 20;
    
    // Title
    std::string title = "=== GALAXY DEBUG INFO ===";
    TextOut(hdc, 20, y, title.c_str(), title.length());
    y += lineHeight * 2;
    
    // Camera info
    std::string cameraHeader = "--- CAMERA ---";
    TextOut(hdc, 20, y, cameraHeader.c_str(), cameraHeader.length());
    y += lineHeight;
    
    std::stringstream ss;
    ss << "Position: (" << std::fixed << std::setprecision(3) 
       << m_cameraPosition.x << ", " << m_cameraPosition.y << ", " << m_cameraPosition.z << ")";
    std::string posText = ss.str();
    TextOut(hdc, 20, y, posText.c_str(), posText.length());
    y += lineHeight;
    
    ss.str("");
    ss << "Speed Multiplier: " << std::fixed << std::setprecision(3) << m_speedMultiplier;
    std::string speedText = ss.str();
    TextOut(hdc, 20, y, speedText.c_str(), speedText.length());
    y += lineHeight * 2;
    
    // Nearest planet info
    std::string planetHeader = "--- NEAREST PLANET ---";
    TextOut(hdc, 20, y, planetHeader.c_str(), planetHeader.length());
    y += lineHeight;
    
    if (m_nearestPlanetIndex >= 0 && m_nearestPlanetIndex < m_planetNames.size()) {
        std::string planetName = m_planetNames[m_nearestPlanetIndex];
        ss.str("");
        ss << "Planet: " << planetName << " (Index " << m_nearestPlanetIndex << ")";
        std::string planetText = ss.str();
        TextOut(hdc, 20, y, planetText.c_str(), planetText.length());
        y += lineHeight;
        
        ss.str("");
        ss << "Distance to Surface: " << std::fixed << std::setprecision(3) << m_nearestDistance << " units";
        std::string distText = ss.str();
        TextOut(hdc, 20, y, distText.c_str(), distText.length());
        y += lineHeight;
        
        // Status
        std::string status;
        if (m_nearestDistance < 0.0f) {
            status = "WARNING: Camera is INSIDE the planet!";
            SetTextColor(hdc, RGB(255, 0, 0)); // Red
        } else if (m_nearestDistance < 1.0f) {
            status = "STATUS: Very close to surface";
            SetTextColor(hdc, RGB(255, 165, 0)); // Orange
        } else if (m_nearestDistance < 10.0f) {
            status = "STATUS: Close to surface";
            SetTextColor(hdc, RGB(0, 128, 0)); // Green
        } else {
            status = "STATUS: Far from surface";
            SetTextColor(hdc, RGB(0, 0, 0)); // Black
        }
        TextOut(hdc, 20, y, status.c_str(), status.length());
        SetTextColor(hdc, RGB(0, 0, 0)); // Reset to black
        y += lineHeight * 2;
    }
    
    // All planets
    std::string allPlanetsHeader = "--- ALL PLANETS ---";
    TextOut(hdc, 20, y, allPlanetsHeader.c_str(), allPlanetsHeader.length());
    y += lineHeight;
    
    for (size_t i = 0; i < m_planetDistances.size() && i < m_planetNames.size(); ++i) {
        std::string planetName = m_planetNames[i];
        ss.str("");
        ss << planetName << ": " << std::fixed << std::setprecision(3) << m_planetDistances[i] << " units";
        if (static_cast<int>(i) == m_nearestPlanetIndex) {
            ss << " [NEAREST]";
        }
        std::string planetText = ss.str();
        TextOut(hdc, 20, y, planetText.c_str(), planetText.length());
        y += lineHeight;
    }
    
    y += lineHeight;
    
    // Controls
    std::string controlsHeader = "--- CONTROLS ---";
    TextOut(hdc, 20, y, controlsHeader.c_str(), controlsHeader.length());
    y += lineHeight;
    
    std::vector<std::string> controls = {
        "WASD: Move camera",
        "Space/Ctrl: Up/Down", 
        "Mouse: Look around",
        "F: Toggle wireframe",
        "ESC: Exit"
    };
    
    for (const auto& control : controls) {
        TextOut(hdc, 20, y, control.c_str(), control.length());
        y += lineHeight;
    }
    
    // Clean up
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void DebugWindow::update(Camera* camera, const std::vector<std::unique_ptr<Planet>>& planets) {
    if (!m_isOpen.load()) return;
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Update camera info
    m_cameraPosition = camera->getPosition();
    m_speedMultiplier = camera->getSpeedMultiplier();
    
    // Find nearest planet and distances
    m_nearestDistance = std::numeric_limits<float>::max();
    m_nearestPlanetIndex = -1;
    m_planetDistances.clear();
    
    for (size_t i = 0; i < planets.size(); ++i) {
        float distanceToSurface = planets[i]->getDistanceToSurface(m_cameraPosition);
        m_planetDistances.push_back(distanceToSurface);
        
        if (distanceToSurface < m_nearestDistance) {
            m_nearestDistance = distanceToSurface;
            m_nearestPlanetIndex = static_cast<int>(i);
        }
    }
}

void DebugWindow::cleanup() {
    m_shouldClose.store(true);
    
    if (m_windowThread.joinable()) {
        m_windowThread.join();
    }
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    m_isOpen.store(false);
}
