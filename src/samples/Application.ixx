/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
export module samples.app;

import std;
import vireo;

export namespace samples {

    class Application {
    public:
        virtual ~Application() = default;

        void init(const vireo::BackendConfiguration config, const vireo::PlatformWindowHandle& windowHandle) {
            this->windowHandle = windowHandle;
            vireo = vireo::Vireo::create(config);
        }

        virtual void onInit() = 0;

        virtual void onUpdate() {}

        virtual void onRender() = 0;

        virtual void onDestroy() = 0;

        virtual void onResize() {}

        virtual void onKeyDown(std::uint32_t key) {}

        virtual void onKeyUp(std::uint32_t key) {}

    protected:
        vireo::PlatformWindowHandle windowHandle;
        std::shared_ptr<vireo::Vireo> vireo;
    };
}