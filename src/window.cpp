#include "window.hpp"
#include <iostream>

void Window::create_window() {
    int monitorID = 0;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    int monitorAmount = 3;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorAmount);
    GLFWmonitor* monitor = monitors[monitorID];
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    window_size.x = mode->width;
    window_size.y = mode->height;

    std::cout << "window resolution: " << window_size.x << ", " << window_size.y << std::endl;

    glfwWindow = glfwCreateWindow(window_size.x, window_size.y, "BlockGame", monitor, nullptr);
    aspect_ratio = window_size.x / window_size.y;
    glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, true);
}

void Window::cursor_setup(bool visible, GLFWwindow* glfwWindow, glm::vec2 window_size) {
    if (visible) {
        glfwSetCursorPos(glfwWindow, static_cast<float>(window_size.x * 0.5f), static_cast<float>(window_size.y * 0.5f));
    }
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, visible ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, visible);
}