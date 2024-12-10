#pragma once

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include <simd/simd.h>
#include <glm/glm.hpp>

#include "window.hpp"

#include "backend/glfw_adapter.h"

struct CameraData {
    simd::float4x4 perspectiveTransform;
    glm::mat4 worldTransform;
    glm::mat3 worldNormalTransform;
};

struct Camera {
    void mouse_setup(GLFWwindow* window);
    void update(MTL::Buffer* pCameraDataBuffer, GLFWwindow* glfwWindow, glm::vec2 window_size, float delta_time, Window* window);
    void on_mouse_move(GLFWwindow* window, glm::vec2 cursor_pos, glm::vec2 window_size);
    CameraData* pCameraData;
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::vec2 rotation = {0.f,0.f};
    float mouse_sens = 0.5f;
    float fov = 45.f;
    glm::mat4 vtrn_mat{1.f};
    glm::mat4 vrot_mat{1.f};
    float delta_time = 0;
    glm::mat4 view_mat;
    glm::vec2 mouse_pos_new = {0.f,0.f};
    glm::vec2 mouse_pos_old = {0.f,0.f};
    double x;
    double y;
    bool esc_pressed = false;
};

