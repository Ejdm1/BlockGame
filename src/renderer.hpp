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


#include <chrono>
#include <string>

struct Renderer {
    void build_shaders(MTL::Device* device);
    void build_buffers(MTL::Device* device);
    void build_frame(MTL::Device* device);
    void build_spencil(MTL::Device* device);
    void build_textures(MTL::Device* device);
    void run();
    static constexpr size_t kMaxFramesInFlight = 3;
    MTL::RenderPipelineState* pRenderPipelineState;
    MTL::Buffer* _pVertexColorsBuffer;
    MTL::Library* _pShaderLibrary;
    MTL::Buffer* _pFrameData[3];
    int _frame = 0;
    dispatch_semaphore_t _semaphore;
    MTL::DepthStencilState* pDepthStencilDescriptor;
    MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* _pnumberOfBlocksInChunkBufferVertex;
    MTL::ArgumentEncoder* pArgumentEncoderFragment;
    MTL::Buffer* pArgumentBufferFragment;
    MTL::Buffer* chunkVertexBuffer;
    MTL::Buffer* _pNumberOfBlocksBufferVertex[kMaxFramesInFlight];
    MTL::Buffer* _pChunkIndexesToBeRenderd[kMaxFramesInFlight];
    MTL::Buffer* _ptexture_side_amountsBufferVertex;
    MTL::Buffer* _ptexture_real_indexBufferVertex;
    MTL::Texture* pTextureArr[128] = {};
    MTL::Function* pFragmentFunction;
    MTL::Function* pVertexFunction;
    float delta_time = 0.f;
    MTL::RenderCommandEncoder* pCommandEncoder;
    bool* p_open;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;

    int instanceCount = 16*16*128;
    std::unordered_map<std::string, int> texturesDict;
    ChunkClass chunkClass;
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