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

import vireo.directx;
import vireo.vulkan;

namespace samples {

    void Application::initRenderingBackEnd(const vireo::RenderingBackends& backendType) {
        if (backendType == vireo::RenderingBackends::VULKAN) {
            renderingBackEnd = make_unique<vireo::VKRenderingBackEnd>(Win32Application::getHwnd());
        } else {
            renderingBackEnd = make_unique<vireo::DXRenderingBackEnd>(Win32Application::getHwnd());
        }
    }

}