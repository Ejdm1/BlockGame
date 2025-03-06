#pragma once

#include <simd/simd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace math {
    constexpr inline glm::vec3 add( const glm::vec3& a, const glm::vec3& b ) {
        return a + b;
    }
 
    constexpr inline glm::mat4 makeIdentity() {
        return glm::mat4(1.0f);
    }

    inline simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar ) {
        glm::mat4 mp = glm::perspective(fovRadians, aspect, znear, zfar);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline simd::float4x4 makeXRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), -angleRadians, glm::vec3{1.0f,0.0f,0.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline simd::float4x4 makeYRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3{0.0f,1.0f,0.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline simd::float4x4 makeZRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), -angleRadians, glm::vec3{0.0f,0.0f,1.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline simd::float4x4 makeTranslate( const glm::vec3& v ) {
        glm::mat4 mp = glm::translate(glm::mat4(1), v);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline simd::float4x4 makeScale( const glm::vec3& v ) {
        glm::mat4 mp = glm::scale(glm::mat4(1.f),v);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline glm::mat3 discardTranslation( const glm::mat4& m ) {
        glm::mat3 mp = glm::mat3(glm::mat4(m));
        return mp;
    }
}