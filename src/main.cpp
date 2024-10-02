#define GLFW_INCLUDE_NONE
#include <glfw/include/GLFW/glfw3.h>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <AppKit/AppKit.hpp>

#include "backend/glfw_adapter.h"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
    }
}

int main() {

    //glfw stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfwWindow = glfwCreateWindow(800, 600, "Heavy", NULL, NULL);

    //Metal Device
    MTL::Device* device = MTL::CreateSystemDefaultDevice();

    //Metal Layer
    CA::MetalLayer* metalLayer = CA::MetalLayer::layer()->retain();
    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);

    //Adapt glfw window to NS Window, and give it a layer to draw to
    NS::Window* window = get_ns_window(glfwWindow, metalLayer)->retain();

    //Drawable Area
    CA::MetalDrawable* metalDrawable;

    //Command Queue
    MTL::CommandQueue* commandQueue = device->newCommandQueue()->retain();

    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwPollEvents();

        glfwSetKeyCallback(glfwWindow, key_callback);

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        metalDrawable = metalLayer->nextDrawable();
        MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
        MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
        colorAttachment->setTexture(metalDrawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setClearColor(MTL::ClearColor(0.75f, 0.25f, 0.125f, 1.0f));
        colorAttachment->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder* renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
        renderCommandEncoder->endEncoding();

        commandBuffer->presentDrawable(metalDrawable);
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();
        pool->release();
    }

    commandQueue->release();
    window->release();
    metalLayer->release();
    glfwTerminate();
    device->release();
    return 0;
}