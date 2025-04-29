/* Copyright (c) 2025-present Henri Michelon
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
export module samples.deferred.global;

export namespace samples {

    constexpr auto RENDER_FORMAT = vireo::ImageFormat::R8G8B8A8_UNORM;
    constexpr auto AXIS_X = vec3{1.0f, 0.0f, 0.0f};
    constexpr auto AXIS_Y = vec3{0.0f, 1.0f, 0.0f};
    constexpr auto AXIS_Z = vec3{0.0f, 0.0f, 1.0f};
    constexpr auto up = AXIS_Y;

#ifdef _WIN32
    enum class KeyCodes : uint32_t {
        LEFT = 37,
        UP = 38,
        RIGHT = 39,
        DOWN = 40,
        P = 80,
    };
#endif

    struct Vertex {
        vec3 pos;
        vec3 color;
    };

    struct Global {
        mat4 projection;
        mat4 view;
    };

    struct Model {
        mat4 transform{1.0f};
    };

} // namespace samples
