/* Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
export module samples.common.global;

export namespace samples {

    constexpr auto AXIS_X = glm::vec3{1.0f, 0.0f, 0.0f};
    constexpr auto AXIS_Y = glm::vec3{0.0f, 1.0f, 0.0f};
    constexpr auto AXIS_Z = glm::vec3{0.0f, 0.0f, 1.0f};
    constexpr auto up = AXIS_Y;

#ifdef _WIN32
    enum class KeyCodes : uint32_t {
        LEFT    = 37,
        UP      = 38,
        RIGHT   = 39,
        DOWN    = 40,
        P       = 80,
        W       = 87,
        S       = 83,
        SPACE   = 32,
    };
#endif

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
    };

    struct Global {
        glm::vec3 cameraPosition{0.0f, 0.0f, 1.2f};
        alignas(16) glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 viewInverse;
        glm::vec4 ambientLight{1.0f, 1.0f, 1.0f, 0.01f}; // RGB + strength
    };

    struct Model {
        glm::mat4 transform{1.0f};
    };

    struct Light {
        glm::vec3 direction{1.0f, -.25f, -.5f};
        alignas(16) glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // RGB + strength
    };

    struct Material {
        float   shininess{128.f};
        int32_t diffuseTextureIndex{-1};
        int32_t normalTextureIndex{-1};
        int32_t aoTextureIndex{-1};
    };

}
