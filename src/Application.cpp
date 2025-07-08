#include "Application.h"
#include <iostream>
#include <GLFW/glfw3.h>

Application::Application() : m_running(false), m_lastFrame(0.0f) {
    m_window = std::make_unique<Window>(1440, 900, "Galaxy CPU - OpenGL Application");
    m_scene = std::make_unique<Scene>();
    m_debugWindow = std::make_unique<DebugWindow>();
}

Application::~Application() {
    cleanup();
}

bool Application::initialize() {
    std::cout << "Initializing Galaxy CPU Application..." << std::endl;
    
    // Initialize window
    if (!m_window->initialize()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }
    
    // Mode souris normal (pas de capture du curseur)
    glfwSetInputMode(m_window->getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Setup mouse callbacks
    m_window->setMouseMovementCallback([this](float xoffset, float yoffset) {
        m_scene->processMouseMovement(xoffset, yoffset);
    });
    
    m_window->setScrollCallback([this](float yoffset) {
        m_scene->getCamera()->processMouseScroll(yoffset);
    });
    
    // Initialize scene
    m_scene->initialize();
    
    // Initialize debug window
    m_debugWindow->initialize();
    
    m_running = true;
    std::cout << "Application initialized successfully!" << std::endl;
    
    return true;
}

void Application::run() {
    std::cout << "Starting main loop..." << std::endl;
    
    while (m_running && !m_window->shouldClose() && !m_debugWindow->shouldClose()) {
        processInput();
        update();
        render();
        
        m_window->swapBuffers();
        m_window->pollEvents();
    }
    
    std::cout << "Application loop ended." << std::endl;
}

void Application::cleanup() {
    std::cout << "Cleaning up application..." << std::endl;
    
    if (m_debugWindow) {
        m_debugWindow->cleanup();
    }
    
    if (m_scene) {
        m_scene->cleanup();
    }
    
    if (m_window) {
        m_window->cleanup();
    }
}

void Application::processInput() {
    // Handle escape key to close application
    if (m_window->isKeyPressed(GLFW_KEY_ESCAPE)) {
        m_running = false;
        m_window->close();
    }
    
    // Show controls on first frame
    static bool showControls = true;
    if (showControls) {
        std::cout << "Controls:" << std::endl;
        std::cout << "  WASD - Move camera" << std::endl;
        std::cout << "  Space - Move up" << std::endl;
        std::cout << "  Ctrl - Move down" << std::endl;
        std::cout << "  Click and drag - Look around" << std::endl;
        std::cout << "  Mouse wheel - Zoom" << std::endl;
        std::cout << "  F - Toggle wireframe" << std::endl;
        std::cout << "  ESC - Exit" << std::endl;
        showControls = false;
    }
    
    // Process camera input
    m_scene->processInput(m_window->getHandle());
}

void Application::update() {
    // Calculate deltaTime
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
    
    // Update scene
    m_scene->update(deltaTime);
    
    // Update debug window with current scene info
    m_debugWindow->update(m_scene->getCamera(), m_scene->getPlanets());
}

void Application::render() {
    // Make sure main window context is current
    glfwMakeContextCurrent(m_window->getHandle());
    
    // Render the scene
    m_scene->render();
    
    // Note: Debug window renders itself in its own thread, no need to call render() here
}
