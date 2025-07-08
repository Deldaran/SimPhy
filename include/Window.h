#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

class Window {
private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    
    // Mouse tracking
    bool m_firstMouse;
    float m_lastX;
    float m_lastY;
    bool m_mousePressed;

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    
    // Callback functions
    std::function<void(float, float)> m_mouseMovementCallback;
    std::function<void(float)> m_scrollCallback;

public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool initialize();
    void cleanup();
    
    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    
    bool isKeyPressed(int key) const;
    void close();
    
    // Mouse callbacks
    void setMouseMovementCallback(std::function<void(float, float)> callback);
    void setScrollCallback(std::function<void(float)> callback);
    
    GLFWwindow* getHandle() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
};
