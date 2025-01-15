#pragma once

#include <cstdio>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "stb_image.h"

#include "backend/glfw_adapter.h"

#include <simd/simd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/backends/imgui_impl_glfw.h"

#define IMGUI_IMPL_METAL_CPP

#include "../dependencies/imgui/backends/imgui_impl_metal.h"

struct Renderer {
    void build_shaders(MTL::Device* device);
    void build_buffers(MTL::Device* device);
    void build_frame(MTL::Device* device);
    void build_spencil(MTL::Device* device);
    void build_textures(MTL::Device* device);
    void run();
    // int blockAmount = 4 * 4;
    static constexpr size_t kMaxFramesInFlight = 3;
    MTL::RenderPipelineState* pRenderPipelineState;
    MTL::Buffer* _pVertexColorsBuffer;
    MTL::Library* _pShaderLibrary;
    MTL::Buffer* _pFrameData[3];
    int _frame = 0;
    dispatch_semaphore_t _semaphore;
    MTL::DepthStencilState* pDepthStencilDescriptor;
    MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* _pVertexDataBuffer;
    MTL::ArgumentEncoder* pArgumentEncoderFragment;
    MTL::Buffer* pArgumentBufferFragment;
    MTL::Buffer* _pArgumentBufferVertex;
    MTL::Texture* pTextureArr[128];
    MTL::Function* pFragmentFunction;
    MTL::Function* pVertexFunction;
    float delta_time = 0.f;
    MTL::RenderCommandEncoder* pCommandEncoder;
    bool* p_open;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;


    int blockID = 4;
    int instanceCount = 16384;
};

static const std::vector<std::string> texture_names = {
"coalore_block", "cobblestone_block", "diamondore_block", "dirt_block", "grass_block", "gravel_block", "ice_block", "ironore_block", "leavesoak_block", "logoak_block", 
"obsidian_block", "planksoak_block", "sand_block", "sandstone_block", "snow_block", "stone_block", "clay_block"
};

static const int texture_side_amounts[47] = {
1, 1, 1, 1, 3, 1, 1, 1, 1, 3, 
1, 1, 1, 3, 1, 1, 1
};

static const std::vector<std::string> texture_end_names = {
    "_bottom",
    "_side",
    "_top"
};

struct ChunkData {
    simd::float3 chunkPosition;
};

struct FrameData {
    float angle;
};

struct VertexData {
    simd::float3 position;
    simd::float3 normal;
    simd::float2 texcoord;
};

struct Blocks {
    int block[6];
};

inline int blockFace(const glm::vec3& pos, int side, int block_id) {
    int data = 0;
    int x, y, z;

    if (pos.x < 0) {
        x = pos.x * -1 + 16;
    }
    else {
        x = pos.x;
    }

    if (pos.y < 0) {
        y = pos.y * -1 + 16;
    }
    else {
        y = pos.y;
    }

    if (pos.z < 0) {
        z = pos.z * -1 + 16;
    }
    else {
        z = pos.z;
    }

    data |= (x & 0x1f) << 0;
    data |= (y & 0x1f) << 5;
    data |= (z & 0x1f) << 10;
    data |= (side & 0x7) << 15;
    data |= (block_id & 0x3fff) << 18;

    return data;
}