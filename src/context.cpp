#include "context.hpp"

void Context::setup(GLFWwindow* glfw_window) {
    ////////Create metal Device//////////
    device = MTL::CreateSystemDefaultDevice();
    /////////////////////////////////////

    /////////////Setup metal Layer/////////////
    metalLayer = CA::MetalLayer::layer()->retain();
    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm_sRGB);
    ///////////////////////////////////////////

    //////Create NS window out of glfw window and get command queue for MTL device/////////
    ns_window = get_ns_window(glfw_window, metalLayer)->retain();

    commandQueue = device->newCommandQueue()->retain();
    ///////////////////////////////////////////////////////////////////////////////////////
}