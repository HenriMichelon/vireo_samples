/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.qt;

import samples.app;

export namespace samples {

    class QtApplication {
    public:
        static int run(
            const std::shared_ptr<Application>& app,
            uint32_t width, uint32_t height,
            const std::string& name,
            int argc, char** argv);

        static auto getWindowHandle() { return windowHandle; }

        static auto& getApp() { return app; }

    private:
        static constexpr auto ID_VULKAN{1001};
        static constexpr auto ID_DIRECTX{1002};

        static PlatformWindowHandle windowHandle;
        static std::shared_ptr<Application> app;

        static bool dirExists(const std::string& dirName);

        // static vireo::Backend backendSelectorDialog(const std::string& title);
    };

}
