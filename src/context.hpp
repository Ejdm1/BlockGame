#pragma once

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "backend/glfw_adapter.h"

struct Context {
    void setup(GLFWwindow* glfw_window);
    MTL::Device* device;
    CA::MetalLayer* metalLayer;
    NS::Window* ns_window;
    CA::MetalDrawable* MetalDrawable;
    MTL::CommandQueue* commandQueue;
}; 