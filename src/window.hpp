#pragma  once

#include <glm/glm.hpp>

#include "backend/glfw_adapter.h"

struct Window {
    void create_window();
    void cursor_setup(bool visible, GLFWwindow* glfwWindow, glm::vec2 window_size);
    GLFWwindow* glfwWindow;
    const glm::vec2 window_size{1280, 720};
    float aspect_ratio;
};