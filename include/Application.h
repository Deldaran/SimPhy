#pragma once
#include "Window.h"
#include "Scene.h"
#include "DebugWindow.h"
#include <memory>

class Application {
private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<DebugWindow> m_debugWindow;
    bool m_running;
    float m_lastFrame;

public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void cleanup();

private:
    void processInput();
    void update();
    void render();
};
