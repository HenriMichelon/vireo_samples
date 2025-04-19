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

        void init(const vireo::Configuration& configuration) {
            vireo = vireo::Vireo::create(configuration);
        }

        virtual void onInit() = 0;

        virtual void onUpdate() {}

        virtual void onRender() = 0;

        virtual void onDestroy() = 0;

        virtual void onResize() {}

        virtual void onKeyDown(uint32_t key) {}

        virtual void onKeyUp(uint32_t key) {}

    protected:
        unique_ptr<vireo::Vireo> vireo;
    };
}