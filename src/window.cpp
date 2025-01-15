#include "window.hpp"
#include <iostream>

void Window::create_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_REFRESH_RATE, 200);

    int monitorAmount = 2;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorAmount);
    GLFWmonitor* monitor;
    if(monitors[1] == NULL) {
        monitor = monitors[0];
    }
    else {
        monitor = monitors[1];
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    window_size.x = mode->width;
    window_size.y = mode->height;

    std::string widthOut = "Window width";
    std::string heightOut = "Window height";

    int spaces = 32 - widthOut.length();
    for (int k = 0; k < spaces; k++) {
        widthOut += "-";
    }
    widthOut = widthOut + "> " + std::to_string(static_cast<int>(window_size.x));

    spaces = 32 - heightOut.length();
    for (int k = 0; k < spaces; k++) {
        heightOut += "-";
    }
    heightOut = heightOut + "> " + std::to_string(static_cast<int>(window_size.y));

    std::cout << widthOut << "\n" << heightOut << std::endl;

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