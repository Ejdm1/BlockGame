#include <metal_stdlib>
using namespace metal;

struct v2f {
    float4 position [[position]];
    float3 normal;
    half3 color;
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

struct BlockData {
    int blockDataArr[6];
};

struct Textures {
    texture2d<half> textures_array[128];
};

struct ChunkData {
    float3 chunkPosition;
};

float MoveBits(int bitAmount, int bitsRight, int data) {
    int shiftedRight = data >> bitsRight;
    int bitMask = (1 << bitAmount) -1;
    int temp = shiftedRight & bitMask;
    if (temp > 15) {
        temp = -temp + 16;
    }
    return static_cast<float>(temp);
}

float3 GetPos(int data) {
    return float3{MoveBits(5, 0, data), MoveBits(5, 5, data), MoveBits(5, 10, data)};
}

int GetSide(int data) {
    return MoveBits(3, 15, data);
}

int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}

v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                        device const CameraData& cameraData [[buffer(1)]],
                        constant BlockData &blockData [[buffer(2)]],
                        uint counter [[vertex_id]] ) {
    v2f o;
    int block_sideID = counter / 6;
    int block_ID = GetBlockId(blockData.blockDataArr[block_sideID]);
    int block_textureID = GetSide(blockData.blockDataArr[block_sideID]);

    const device VertexData& vd = vertexData[ counter ];
    float4 pos = float4( vd.position, 1.0 );
    o.position = cameraData.perspectiveTransform * cameraData.worldTransform * pos;

    o.texcoord = vd.texcoord.xy;
    switch (block_textureID) {
        case 0:
            o.side = 0;
            break;
        case 4:
            o.side = 1;
            break;
        case 5:
            o.side = 2;
            break;
    }

    return o;
}

half4 fragment fragmentMain( v2f in [[stage_in]], constant Textures &textures [[buffer(0)]] ) {
    constexpr sampler s( address::repeat, filter::nearest );

    half3 texel = textures.textures_array[in.side].sample( s, in.texcoord ).rgb;

    return half4( texel, 1.0 );
}