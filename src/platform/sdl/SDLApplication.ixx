/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.sdl;

import samples.app;

export namespace samples {

    class SDLApplication {
    public:
        static int run(
            std::shared_ptr<Application> app,
            std::uint32_t width, std::uint32_t height,
            const std::string& name);

        static auto getWindowHandle() { return windowHandle; }

        static auto& getApp() { return app; }

    private:
        static vireo::PlatformWindowHandle windowHandle;
        static std::shared_ptr<Application> app;

        static bool dirExists(const std::string& dirName);
    };

}
