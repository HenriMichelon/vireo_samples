/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.cube;

import samples.app;
import samples.cube.depthprepass;
import samples.cube.scene;
import samples.cube.skybox;
import samples.cube.postprocessing;
import samples.cube.colorpass;

export namespace samples {

    class CubeApp : public Application {
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

        bool                           applyPostProcessing{false};
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