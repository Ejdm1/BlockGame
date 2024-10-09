#pragma once

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include <simd/simd.h>
#include <glm/glm.hpp>

#include "window.cpp"

#include "backend/glfw_adapter.h"

struct CameraData {
    simd::float4x4 perspectiveTransform;
    glm::mat4 worldTransform;
    simd::float3x3 worldNormalTransform;
};

struct Camera {
    void update(MTL::Buffer* pCameraDataBuffer, GLFWwindow* glfwWindow, glm::vec2 window_size, float delta_time, Window* window);
    void on_mouse_move(GLFWwindow* window, double x, double y, glm::vec2 window_size);
    CameraData* pCameraData;

    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::vec2 rotation = {0.f,0.f};
    float mouse_sens = 0.5f;
    float fov = 45.f;
    glm::mat4 vtrn_mat{1.f};
    glm::mat4 vrot_mat{1.f};
    float delta_time = 0;
    glm::mat4 view_mat;
};

