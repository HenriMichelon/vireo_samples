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
import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;
import samples.common.skybox;
import samples.common.postprocessing;
import samples.common.samplers;
import samples.deferred.gbuffer;
import samples.deferred.lightingpass;
import samples.deferred.oitpass;

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
        static constexpr auto RENDER_FORMAT = vireo::ImageFormat::R8G8B8A8_UNORM;

        struct FrameData : FrameDataCommand {
            std::shared_ptr<vireo::Fence>        inFlightFence;
            std::shared_ptr<vireo::RenderTarget> colorBuffer;
            std::shared_ptr<vireo::Semaphore>    semaphore;
        };

        Scene                               scene;
        DepthPrepass                        depthPrepass;
        Skybox                              skybox;
        PostProcessing                      postProcessing;
        GBufferPass                         gbufferPass;
        LightingPass                        lightingPass;
        TransparencyPass                    transparencyPass;
        Samplers                            samplers;
        std::vector<FrameData>              framesData;
        std::shared_ptr<vireo::SwapChain>   swapChain;
        std::shared_ptr<vireo::SubmitQueue> graphicQueue;
    };
}