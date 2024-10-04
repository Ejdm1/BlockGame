#include <simd/simd.h>

namespace math
{
    constexpr simd::float3 add( const simd::float3& a, const simd::float3& b );
    constexpr simd_float4x4 makeIdentity();
    simd::float4x4 makePerspective();
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
}

namespace shader_types
{
    struct InstanceData
    {
        simd::float4x4 instanceTransform;
        simd::float4 instanceColor;
    };

    struct CameraData
    {
        simd::float4x4 perspectiveTransform;
        simd::float4x4 worldTransform;
    };
}