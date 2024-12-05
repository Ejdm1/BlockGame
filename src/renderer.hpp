#pragma once

#include <cstdio>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "stb_image.h"

#include "backend/glfw_adapter.h"

#include <simd/simd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

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
    MTL::Buffer* _pBlockDataBuffer;
    MTL::Buffer* _pBlockSideBuffer;
    MTL::Texture* _pTexture[10];
    MTL::Function* pFragFn;
    float delta_time = 0.f;
};

struct FrameData {
    float angle;
};

struct InstanceData {
    simd::float4x4 instanceTransform;
    glm::mat3 instanceNormalTransform;
    simd::float4 instanceColor;
};

struct VertexData {
    simd::float3 position;
    simd::float3 normal;
    simd::float2 texcoord;
};

enum struct BlockID : int {
    grass_top,
    grass_side,
    grass_bottom
};

enum struct SideID : int {
    Top,
    Side,
    bot
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

struct BlockData {
    int bot = blockFace(glm::vec3 {0,-1,0}, 2, 0);
    int top = blockFace(glm::vec3 {0,1,0}, 0, 0);
    int sideFront = blockFace(glm::vec3 {0,0,1}, 1, 0);
    int sideBack = blockFace(glm::vec3 {0,0,-1}, 1, 0);
    int sideRight = blockFace(glm::vec3 {-1,0,0}, 1, 0);
    int sideLeft = blockFace(glm::vec3 {1,0,0}, 1, 0);
};