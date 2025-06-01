#pragma once

#include <cstdio>

#include "stb_image.h"

#include <glm/gtc/matrix_transform.hpp>

#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/backends/imgui_impl_glfw.h"
#define IMGUI_IMPL_METAL_CPP
#include "../dependencies/imgui/backends/imgui_impl_metal.h"

#include "camera.hpp"
#include "context.hpp"
#include "window.hpp"
#include "chunk.hpp"
#include "math.hpp"

#include <string>

struct Renderer {
    void build_shaders(MTL::Device* device);
    void build_buffers(MTL::Device* device, bool first, int chunkLine);
    void build_spencil(MTL::Device* device);
    void build_textures(MTL::Device* device);
    void GetMaps();
    void run();
    static constexpr size_t kMaxFramesInFlight = 3;
    MTL::RenderPipelineState* RenderPipelineState;
    MTL::DepthStencilState* DepthStencilState;
    int frame = 0;
    int mapIndex = 0;
    bool regenerate = false;
    dispatch_semaphore_t _semaphore;
    MTL::ArgumentEncoder* ArgumentEncoderFragment;
    MTL::Buffer* ArgumentBufferFragment;
    MTL::Buffer* CameraDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* NumberOfBlocksBufferVertex[kMaxFramesInFlight];
    MTL::Buffer* ChunkIndexesToBeRenderd[kMaxFramesInFlight];
    MTL::Buffer* mapBuffers[2];
    MTL::Buffer* texture_side_amounts_buffer_vertex;
    MTL::Buffer* texture_real_index_buffer_vertex;
    MTL::Texture* TextureArr[128] = {};
    MTL::Function* FragmentFunction;
    MTL::Function* VertexFunction;
    float delta_time = 0.f;
    MTL::RenderCommandEncoder* CommandEncoder;
    bool* open;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
    std::vector<int> chunkIndexToKeep = {};

    int instanceCount = 16*16*128;
    std::unordered_map<std::string, int> texturesDict;
    ChunkClass chunkClass;
    bool inAppRegenerate = false;
    bool loadMap = false;
    std::vector<std::string> names;
    std::vector<int> seeds;
};

static const std::vector<std::string> texture_end_names = {
    "_bottom",
    "_side",
    "_top"
};

struct Texture_side_amounts {
    int texture_side_amounts[128] = {};
};

struct Texture_real_index {
    int texture_real_index[128] = {};
};