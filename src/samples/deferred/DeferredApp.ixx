/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred;

import samples.app;
import samples.common.depthprepass;
import samples.common.scene;
import samples.common.skybox;
import samples.common.postprocessing;
import samples.deferred.colorpass;

export namespace samples {

    class DeferredApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onResize() override;
        void onDestroy() override;
        void onUpdate() override;
        void onKeyDown(uint32_t key) override;

    private:
        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::RenderTarget>     colorBuffer;
        };

        vector<FrameData>              framesData;
        shared_ptr<vireo::SwapChain>   swapChain;
        shared_ptr<vireo::SubmitQueue> graphicQueue;
        Scene                          scene;
        DepthPrepass                   depthPrepass;
        Skybox                         skybox;
        PostProcessing                 postProcessing;
        ColorPass                      colorPass;
    };
}