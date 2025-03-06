#pragma  once

#include <glm/glm.hpp>

#include "backend/glfw_adapter.h"
#include <iostream>

struct Window {
    void create_window();
    void cursor_setup(bool visible, GLFWwindow* glfwWindow, glm::vec2 window_size);
    GLFWwindow* glfwWindow;
    void cursor_release(GLFWwindow* glfwWindow, glm::vec2 window_size);
    glm::vec2 window_size{};
    float aspect_ratio;
    bool menu = false;
    bool options = false;
};