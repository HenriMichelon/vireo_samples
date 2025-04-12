#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "backend/vulkan/Tools.h"

import vireo.backend.vulkan;

namespace vireo {

    TEST_CASE("RAL/Vulkan", "Rendering backend instance") {
        const auto backend = std::make_unique<backend::VKRenderingBackEnd>();
    }

}