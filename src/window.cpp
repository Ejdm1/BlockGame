#include "window.hpp"

void Window::create_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(window_size.x, window_size.y, "Heavy", nullptr, nullptr);
}

void Window::cursor_setup(bool visible, GLFWwindow* glfwWindow, glm::vec2 window_size) {
    glfwSetCursorPos(glfwWindow, static_cast<float>(window_size.x / 2), static_cast<float>(window_size.y / 2));
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, visible ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, visible);
}