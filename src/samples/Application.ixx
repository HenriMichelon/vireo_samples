/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.app;

export namespace samples {

    class Application {
    public:
        virtual ~Application() = default;

        void initRenderingBackEnd(const backend::RenderingBackends& backendType);

        virtual void onInit() = 0;
        virtual void onUpdate() {}
        virtual void onRender() = 0;
        virtual void onDestroy() = 0;

        virtual void onKeyDown(uint32_t key)   {}
        virtual void onKeyUp(uint32_t key)     {}

    protected:
        std::unique_ptr<backend::RenderingBackEnd> renderingBackEnd;
    };
}