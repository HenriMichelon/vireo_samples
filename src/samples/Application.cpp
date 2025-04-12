/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.app;

import samples.win32;

import vireo.backend.directx;
import vireo.backend.vulkan;

namespace samples {

    void Application::initRenderingBackEnd(const backend::RenderingBackends& backendType) {
        if (backendType == backend::RenderingBackends::VULKAN) {
            renderingBackEnd = std::make_unique<backend::VKRenderingBackEnd>(Win32Application::getHwnd());
        } else {
            renderingBackEnd = std::make_unique<backend::DXRenderingBackEnd>(Win32Application::getHwnd());
        }
    }

}