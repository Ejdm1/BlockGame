#pragma once

#include <cstdio>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "stb_image.h"

#include "backend/glfw_adapter.h"

#include <simd/simd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Renderer {
    void build_shaders(MTL::Device* device);
    void build_buffers(MTL::Device* device);
    void build_frame(MTL::Device* device);
    void build_spencil(MTL::Device* device);
    void build_textures(MTL::Device* device);
    void run();
    static constexpr size_t kInstanceRows = 1;
    static constexpr size_t kInstanceColumns = 1;
    static constexpr size_t kInstanceDepth = 1;
    static constexpr size_t kNumInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);
    static constexpr size_t kMaxFramesInFlight = 3;
    MTL::RenderPipelineState* _pPSO;
    MTL::Buffer* _pVertexColorsBuffer;
    MTL::Library* _pShaderLibrary;
    MTL::Buffer* _pFrameData[3];
    int _frame = 0;
    dispatch_semaphore_t _semaphore;
    MTL::DepthStencilState* _pDepthStencilState;
    MTL::Buffer* _pInstanceDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* _pIndexBuffer;
    MTL::Buffer* _pVertexDataBuffer;
    MTL::Texture* _pTexture;
    float delta_time = 0.f;
};

struct FrameData {
    float angle;
};

struct InstanceData {
    simd::float4x4 instanceTransform;
    simd::float3x3 instanceNormalTransform;
    simd::float4 instanceColor;
};

struct VertexData {
    simd::float3 position;
    simd::float3 normal;
    simd::float2 texcoord;
};