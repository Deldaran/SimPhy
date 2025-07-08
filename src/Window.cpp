#include "Window.h"
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title),
      m_firstMouse(true), m_lastX(width / 2.0f), m_lastY(height / 2.0f), m_mousePressed(false) {
}

Window::~Window() {
    cleanup();
}

bool Window::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    if (m_window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(m_window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set the viewport
    glViewport(0, 0, m_width, m_height);

    // Set callback for window resize
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    
    // Set mouse callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    
    // Disable cursor for FPS-like camera
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;

    return true;
}

void Window::cleanup() {
    if (m_window) {
        glfwTerminate();
        m_window = nullptr;
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) const {
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void Window::close() {
    glfwSetWindowShouldClose(m_window, true);
}

// Static callback function
void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Window::setMouseMovementCallback(std::function<void(float, float)> callback) {
    m_mouseMovementCallback = callback;
}

void Window::setScrollCallback(std::function<void(float)> callback) {
    m_scrollCallback = callback;
}

void Window::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    
    if (win->m_firstMouse) {
        win->m_lastX = xpos;
        win->m_lastY = ypos;
        win->m_firstMouse = false;
    }
    
    float xoffset = xpos - win->m_lastX;
    float yoffset = win->m_lastY - ypos; // Reversed since y-coordinates go from bottom to top
    
    win->m_lastX = xpos;
    win->m_lastY = ypos;
    
    // Appeler le callback seulement si le bouton de la souris est pressé
    if (win->m_mousePressed && win->m_mouseMovementCallback) {
        win->m_mouseMovementCallback(xoffset, yoffset);
    }
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            win->m_mousePressed = true;
        } else if (action == GLFW_RELEASE) {
            win->m_mousePressed = false;
        }
    }
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win->m_scrollCallback) {
        win->m_scrollCallback(yoffset);
    }
}
