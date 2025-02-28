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
    void Regenerate();
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
    MTL::Buffer* _pnumberOfBlocksInChunkBufferVertex;
    MTL::ArgumentEncoder* pArgumentEncoderFragment;
    MTL::Buffer* pArgumentBufferFragment;
    MTL::Buffer* _pArgumentBufferVertex;
    MTL::Buffer* _pNumberOfBlocksBufferVertex;
    MTL::Buffer* _ptexture_side_amountsBufferVertex;
    MTL::Buffer* _ptexture_real_indexBufferVertex;
    MTL::Texture* pTextureArr[128];
    MTL::Function* pFragmentFunction;
    MTL::Function* pVertexFunction;
    float delta_time = 0.f;
    MTL::RenderCommandEncoder* pCommandEncoder;
    bool* p_open;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;

    bool regenerate = false;
    int chunkLine = 16;
    std::string mapFileName = "map" + std::to_string(chunkLine) + "x" + std::to_string(chunkLine);
    int chunkCount = chunkLine * chunkLine;
    int instanceCount = 16*16*256;
    int blockCounter = 0;
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

struct FrameData {
    float angle;
};

struct VertexData {
    simd::float3 position;
    simd::float3 normal;
    simd::float2 texcoord;
};

struct Block {
    int block;
};

struct Chunk {
    int chunkPos;
    int blocks[256][16][16];
};

struct ChunkToGPU {
    int chunkPos;
    int blocks [16*16*256];
};

struct NuberOfBlocksInChunk {
    int nuberOfBlocks[1024] = {};
};

//Saveing block data into single int x,y,z,blockID(4bits x, 8bits y, 4bits z, 14bits blockID)//
inline int blockFace(glm::vec3 pos, int block_id) {
    int data = 0;
    if (pos.x > 15) {pos.x = 0;}
    if (pos.y > 255) {pos.y = 0;}
    if (pos.z > 15) {pos.z = 0;}

    data |= ((int)pos.x & 0xf) << 0;
    data |= ((int)pos.y & 0xff) << 4;
    data |= ((int)pos.z & 0xf) << 12;
    data |= (block_id & 0x3fff) << 18;

    return data;
}
///////////////////////////////////////////////////////////////////////////////////////////////