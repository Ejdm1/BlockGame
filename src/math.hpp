#pragma once

#include <simd/simd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace math {
    inline simd::float4x4 makePerspective(float fovRadians, float aspect, float znear, float zfar) {
        glm::mat4 mp = glm::perspective(fovRadians, aspect, znear, zfar);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    inline glm::mat3 discardTranslation(const glm::mat4& m) {
        glm::mat3 mp = glm::mat3(glm::mat4(m));
        return mp;
    }
}