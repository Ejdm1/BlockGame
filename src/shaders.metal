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

struct InstanceData {
    float4x4 instanceTransform;
    float3x3 instanceNormalTransform;
    float4 instanceColor;
};

struct CameraData {
    float4x4 perspectiveTransform;
    float4x4 worldTransform;
    float3x3 worldNormalTransform;
};

struct Textures {
    texture2d<half> textures_array[128];
};

struct BlockData {
    int top, bot, sideFront, sideBack, sideRight, sideLeft;
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

simd::float3 GetPos(int data) {
    return simd::float3{MoveBits(5, 0, data), MoveBits(5, 5, data), MoveBits(5, 10, data)};
}

int GetSide(int data) {
    return MoveBits(3, 15, data);
}

int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}

v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                        device const InstanceData* instanceData [[buffer(1)]],
                        device const CameraData& cameraData [[buffer(2)]],
                        device const BlockData* blockData [[buffer(3)]],
                        uint vertexId [[vertex_id]],
                        uint instanceId [[instance_id]] ) {
    v2f o;

    const device VertexData& vd = vertexData[ vertexId ];
    float4 pos = float4( vd.position, 1.0 );
    pos = instanceData[ instanceId ].instanceTransform * pos;
    pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
    o.position = pos;

    float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
    normal = cameraData.worldNormalTransform * normal;
    o.normal = normal;

    o.texcoord = vd.texcoord.xy;

    o.color = half3( instanceData[ instanceId ].instanceColor.rgb );

    BlockData bd = blockData[ 0 ];

    o.side = GetSide(bd.sideFront);
    return o;
}

half4 fragment fragmentMain( v2f in [[stage_in]], constant Textures &textures [[buffer(0)]] ) {
    constexpr sampler s( address::repeat, filter::linear );

    half3 texel = textures.textures_array[in.side].sample( s, in.texcoord ).rgb;

    // assume light coming from (front-top-right)
    float3 l = normalize(float3( 1.0, 1.0, 1.0 ));
    float3 n = normalize( in.normal );

    half ndotl = half( saturate( dot( n, l ) ) );

    half3 illum = (in.color * texel * 0.1) + (in.color * texel * ndotl);
    return half4( illum, 1.0 );
}