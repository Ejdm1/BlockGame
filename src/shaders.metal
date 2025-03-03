#include <metal_stdlib>
using namespace metal;

struct v2f {
    float4 position [[position]];
    float color;
    float2 texcoord;
    int side;
    bool del = false;
};

struct VertexData {
    float3 position;
    float3 normal;
    float2 texcoord;
};

struct CameraData {
    float4x4 perspectiveTransform;
    float4x4 worldTransform;
    float3x3 worldNormalTransform;
};

struct Chunk {
    int chunkPos;
    int blocks[256*16*16];
};

struct Textures {
    texture2d<half> pTextureArr[128];
};

struct NuberOfBlocksInChunk {
    int nuberOfBlocks[1024];
};

struct Texture_real_index {
    int texture_real_index[128];
};

struct Texture_side_amounts {
    int texture_side_amounts[128];
};

float MoveBits(int bitAmount, int bitsRight, int data) {
    int shiftedRight = data >> bitsRight;
    int bitMask = (1 << bitAmount) -1;
    int temp = shiftedRight & bitMask;
    return static_cast<float>(temp);
}

float2 GetChunkPosition(int pos) {
    float2 posVec;
    posVec.y = MoveBits(8,0,pos);
    posVec.x = MoveBits(8,8,pos);
    if(posVec.x > 128) {
        posVec.x = (posVec.x * -1) + 128;
    };
    if(posVec.y > 128) {
        posVec.y = (posVec.y * -1) + 128;
    };
    return posVec;
}

float3 GetPos(int data) {
    return float3{MoveBits(4, 0, data), MoveBits(8, 4, data), MoveBits(4, 12, data)};
}

int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}

constant VertexData verts[36] = {
    //                                               Texture
    //      Positions            Normals           Coordinates
    { { +0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
    { { -0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
    { { -0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
    { { -0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },//back
    { { +0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },
    { { +0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
    
    { { -0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
    { { +0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
    { { +0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
    { { +0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },//front
    { { -0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },
    { { -0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },

    { { +0.5, -0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { +0.5, -0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { +0.5, +0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { +0.5, +0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },//right
    { { +0.5, +0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },
    { { +0.5, -0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },

    { { -0.5, -0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { -0.5, -0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { -0.5, +0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { -0.5, +0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },//left
    { { -0.5, +0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },
    { { -0.5, -0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },

    { { -0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
    { { +0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
    { { +0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
    { { +0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },//top
    { { -0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },
    { { -0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },

    { { -0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
    { { +0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
    { { +0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
    { { +0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },//bottom
    { { -0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } },
    { { -0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } }
};

constant VertexData cross_verts[36] = {
    //                                               Texture
    //      Positions            Normals           Coordinates
    { { +0.4, -0.5, +0.4 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
    { { -0.4, -0.5, -0.4 }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
    { { -0.4, +0.5, -0.4 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
    { { -0.4, +0.5, -0.4 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },//back
    { { +0.4, +0.5, +0.4 }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },
    { { +0.4, -0.5, +0.4 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
    
    { { -0.4, -0.5, +0.4 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
    { { +0.4, -0.5, -0.4 }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
    { { +0.4, +0.5, -0.4 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
    { { +0.4, +0.5, -0.4 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },//front
    { { -0.4, +0.5, +0.4 }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },
    { { -0.4, -0.5, +0.4 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },

    { { +0.4, -0.5, -0.4 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { -0.4, -0.5, +0.4 }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { -0.4, +0.5, +0.4 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { -0.4, +0.5, +0.4 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },//right
    { { +0.4, +0.5, -0.4 }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },
    { { +0.4, -0.5, -0.4 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },

    { { -0.4, -0.5, -0.4 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
    { { +0.4, -0.5, +0.4 }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
    { { +0.4, +0.5, +0.4 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
    { { +0.4, +0.5, +0.4 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },//left
    { { -0.4, +0.5, -0.4 }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },
    { { -0.4, -0.5, -0.4 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },

    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },

    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } },
    { { 0.f,0.f,0.f }, {  0.f, 0.f, 0.f }, { 0.f, 0.f } }
};

v2f vertex vertexMain(  device const CameraData& cameraData [[buffer(1)]],
                        device const  Chunk* chunkIn [[buffer(2)]],
                        device const NuberOfBlocksInChunk* nuberOfBlocksInChunk [[buffer(3)]],
                        device const Texture_real_index* texture_real_index [[buffer(4)]],
                        device const Texture_side_amounts* texture_side_amounts [[buffer(5)]],
                        uint counter [[vertex_id]],
                        uint instance [[instance_id]]) {
    v2f o;
    int blockNumber = 0;
    int chunkIndex = 0;
    int blockIndex = 0;
    while(instance > blockNumber) {
        if(blockNumber + nuberOfBlocksInChunk->nuberOfBlocks[chunkIndex] > instance) {
            break;
        }
        else {
            blockNumber = blockNumber + nuberOfBlocksInChunk->nuberOfBlocks[chunkIndex];
            chunkIndex++;
        }
    }

    blockIndex = instance - blockNumber;
    const device Chunk& chunk = chunkIn[chunkIndex];

    int block_sideID = counter/6;

    int block_ID = GetBlockId(chunk.blocks[blockIndex]);
    VertexData vd;
    if(block_ID == 16) {
        if(counter > 23) {
            o.del = true;
        }
        vd = cross_verts[counter];
    }
    else {
        vd = verts[counter];
    }

    float3 blockPos = GetPos(chunk.blocks[blockIndex]);
    float2 chunkPos = GetChunkPosition(chunk.chunkPos);
    float4 pos = float4(vd.position + blockPos + float3(0,-50,0) + float3(chunkPos.x * 16,0,chunkPos.y * 16), 1.0);
    o.position = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
    o.texcoord = vd.texcoord.xy;

    switch (texture_side_amounts->texture_side_amounts[block_ID]) {
        case 3:
            switch (block_sideID) {
                case 0:
                    o.side = texture_real_index->texture_real_index[block_ID]+1;
                    break;
                case 1:
                    o.side = texture_real_index->texture_real_index[block_ID]+1;
                    break;
                case 2:
                    o.side = texture_real_index->texture_real_index[block_ID]+1;
                    break;
                case 3:
                    o.side = texture_real_index->texture_real_index[block_ID]+1;
                    break;
                case 4:
                    o.side = texture_real_index->texture_real_index[block_ID]+2;
                    break;
                case 5:
                    o.side = texture_real_index->texture_real_index[block_ID];
                    break;
            }
            break;
        case 1:
            o.side = texture_real_index->texture_real_index[block_ID];
            break;
    }

    return o;
}

half4 fragment fragmentMain(v2f in [[stage_in]], device Textures &textures [[buffer(0)]]) {
    sampler s;
    if(in.side == 20) {
        s = sampler(address::repeat, mag_filter::nearest, min_filter::nearest, mip_filter::none, lod_clamp(0.0, 0.0));
    }
    else {
        s = sampler(address::repeat, mag_filter::nearest, min_filter::linear, mip_filter::linear, lod_clamp(0.0, 10.0));
    }

    half4 texel = textures.pTextureArr[in.side].sample(s, in.texcoord);

    if(texel.a < 0.1 || in.del) {discard_fragment();}

    return half4(texel);
}