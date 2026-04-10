/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
export module samples.deferred;

import std;
import vireo;
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
        void onKeyDown(std::uint32_t key) override;

    private:
        static constexpr auto RENDER_FORMAT = vireo::ImageFormat::R8G8B8A8_UNORM;
        // static constexpr auto RENDER_FORMAT = vireo::ImageFormat::B8G8R8A8_UNORM; // X11

        struct FrameData : FrameDataCommand {
            std::shared_ptr<vireo::Fence>        inFlightFence;
            std::shared_ptr<vireo::RenderTarget> colorBuffer;
            std::shared_ptr<vireo::Semaphore>    semaphore;
            std::shared_ptr<vireo::QueryPool>    lastQueryPool;
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