#pragma once

#include <simd/simd.h>
#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <glfw/include/GLFW/glfw3.h>

namespace math {
    constexpr simd::float3 add( const simd::float3& a, const simd::float3& b );
    constexpr glm::mat4 makeIdentity();
    simd::float4x4 makePerspective();
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
    simd::float3x3 discardTranslation( const glm::mat4& m );
}

namespace shader_types {
    struct VertexData {
        simd::float3 position;
        simd::float3 normal;
        simd::float2 texcoord;
    };

    struct InstanceData {
        simd::float4x4 instanceTransform;
        simd::float3x3 instanceNormalTransform;
        simd::float4 instanceColor;
    };

    struct CameraData {
        simd::float4x4 perspectiveTransform;
        glm::mat4 worldTransform;
        simd::float3x3 worldNormalTransform;
    };
}

namespace input {
    struct Keybinds {
        int move_pz, move_nz;
        int move_px, move_nx;
        int move_py, move_ny;
        int toggle_sprint;
    };

    static inline constexpr Keybinds DEFAULT_KEYBINDS {
        .move_pz = GLFW_KEY_W,
        .move_nz = GLFW_KEY_S,
        .move_px = GLFW_KEY_A,
        .move_nx = GLFW_KEY_D,
        .move_py = GLFW_KEY_SPACE,
        .move_ny = GLFW_KEY_LEFT_CONTROL,
        .toggle_sprint = GLFW_KEY_LEFT_SHIFT,
    };
}