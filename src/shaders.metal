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
    int Blocks[16384][6];
};

struct Textures {
    texture2d<half> pTextureArr[128];
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

constant int texture_side_amounts[47] = {
1, 1, 1, 1, 3, 1, 1, 1, 1, 3, 
1, 1, 1, 3, 1, 1
};

constant int texture_real_index[47] = {
0, 1, 2, 3, 4, 7, 8, 9, 10, 11, 
14, 15, 16, 17, 20, 21
};

v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                        device const CameraData& cameraData [[buffer(1)]],
                        constant BlockData &blockData [[buffer(2)]],
                        uint counter [[vertex_id]],
                        uint instance [[instance_id]] ) {
    v2f o;
    int block_sideID = (counter - (36 * (instance))) / 6;
    int block_ID = GetBlockId(blockData.Blocks[instance][block_sideID]);

    const device VertexData& vd = vertexData[ counter % 36 ];
    float3 blockPos = GetPos(blockData.Blocks[instance][block_sideID]);
    float4 pos = float4( vd.position + float3((instance/128), -2 ,(instance % 128)) - float3(64,0,64), 1.0 );
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

half4 fragment fragmentMain( v2f in [[stage_in]], device Textures &textures [[buffer(0)]] ) {
    constexpr sampler s( address::repeat, filter::nearest );

    half4 texel = textures.pTextureArr[in.side].sample( s, in.texcoord );

    return half4( texel );
}