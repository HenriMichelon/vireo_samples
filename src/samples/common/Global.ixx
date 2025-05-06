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
    constexpr auto AXIS_UP = AXIS_Y;

#ifdef _WIN32
    enum class KeyScanCodes : uint32_t {
        LEFT    = 75,
        UP      = 72,
        RIGHT   = 77,
        DOWN    = 80,
        M       = 39,
        F       = 33,
        P       = 25,
        G       = 34,
        W       = 17,
        A       = 30,
        S       = 31,
        D       = 32,
        SPACE   = 57,
    };
#endif

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
    };

    struct Global {
        glm::vec3 cameraPosition{0.0f, 0.0f, 1.75f};
        alignas(16) glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 viewInverse;
        glm::vec4 ambientLight{1.0f, 1.0f, 1.0f, 0.01f}; // RGB + strength
    };

    struct Model {
        glm::mat4 transform{1.0f};
    };

    struct Light {
        alignas(16) glm::vec3 direction{1.0f, -.25f, -.5f};
        alignas(16) glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // RGB + strength
    };

    struct Material {
        alignas(16) float  shininess{128.f};
        alignas(4) int32_t diffuseTextureIndex{-1};
        alignas(4) int32_t normalTextureIndex{-1};
        alignas(4) int32_t aoTextureIndex{-1};
    };

    struct FrameDataCommand {
        std::shared_ptr<vireo::CommandAllocator> commandAllocator;
        std::shared_ptr<vireo::CommandList>      commandList;
    };

}
