#include "context.hpp"

void Context::setup(GLFWwindow* glfw_window) {
    //Metal Device
    device = MTL::CreateSystemDefaultDevice();

    //Metal Layer
    metalLayer = CA::MetalLayer::layer()->retain();
    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm_sRGB);

    //Adapt glfw window to NS Window and give it a layer to draw on
    ns_window = get_ns_window(glfw_window, metalLayer)->retain();

    commandQueue = device->newCommandQueue()->retain();
}