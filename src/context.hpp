#pragma once

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "backend/glfw_adapter.h"

struct Context {
    void setup(GLFWwindow* glfw_window);
    MTL::Device* device;
    CA::MetalLayer* metalLayer;
    NS::Window* window;
    CA::MetalDrawable* metalDrawable;
    MTL::CommandQueue* commandQueue;
};