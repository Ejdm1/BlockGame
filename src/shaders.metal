#include <metal_stdlib>
using namespace metal;

struct v2f {
    float4 position [[position]];
    float color;
    float2 texcoord;
    int side;
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
    int nuberOfBlocks[256];
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

constant int texture_side_amounts[16] = {
1, 1, 1, 1, 3, 1, 1, 1, 1, 3, 
1, 1, 1, 3, 1, 1
};

constant int texture_real_index[16] = {
0, 1, 2, 3, 4, 7, 8, 9, 10, 11, 
14, 15, 16, 17, 20, 21
};

v2f vertex vertexMain(  device const VertexData* vertexData [[buffer(0)]],
                        device const CameraData& cameraData [[buffer(1)]],
                        device const  Chunk* chunkIn [[buffer(2)]],
                        device const NuberOfBlocksInChunk* nuberOfBlocksInChunk [[buffer(3)]],
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

    const device VertexData& vd = vertexData[counter];
    float3 blockPos = GetPos(chunk.blocks[blockIndex]);
    float2 chunkPos = GetChunkPosition(chunk.chunkPos);
    float4 pos = float4(vd.position + blockPos + float3(0,-127,0) + float3(chunkPos.x * 16,0,chunkPos.y * 16), 1.0);
    o.position = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
    o.texcoord = vd.texcoord.xy;

    switch (texture_side_amounts[block_ID]) {
        case 3:
            switch (block_sideID) {
                case 0:
                    o.side = texture_real_index[block_ID]+1;
                    break;
                case 1:
                    o.side = texture_real_index[block_ID]+1;
                    break;
                case 2:
                    o.side = texture_real_index[block_ID]+1;
                    break;
                case 3:
                    o.side = texture_real_index[block_ID]+1;
                    break;
                case 4:
                    o.side = texture_real_index[block_ID]+2;
                    break;
                case 5:
                    o.side = texture_real_index[block_ID];
                    break;
            }
            break;
        case 1:
            o.side = texture_real_index[block_ID];
            break;
    }

    return o;
}

half4 fragment fragmentMain(v2f in [[stage_in]], device Textures &textures [[buffer(0)]]) {
    constexpr sampler s(address::repeat, mag_filter::nearest, min_filter::linear);

    half4 texel = textures.pTextureArr[in.side].sample(s, in.texcoord);

    if(texel.a < 0.1) {discard_fragment();}

    return half4(texel);
    // return half4(half3(in.color,in.color,in.color), 1.0f);
}