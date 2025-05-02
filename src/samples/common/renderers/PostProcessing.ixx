/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.common.postprocessing;

import samples.common.global;

export namespace samples {

    class PostProcessing {
    public:
        void onUpdate();
        void onInit(
            const shared_ptr<vireo::Vireo>& vireo,
            vireo::ImageFormat renderFormat,
            uint32_t framesInFlight);
        void onResize(const vireo::Extent& extent);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const shared_ptr<vireo::CommandList>& cmdList,
            const shared_ptr<vireo::RenderTarget>& colorBuffer);

        auto getColorBuffer(const uint32_t frameIndex) const {
            return applyGammaCorrection ? framesData[frameIndex].gammaCorrectionColorBuffer :
                   applyEffect ? framesData[frameIndex].effectColorBuffer :
                   framesData[frameIndex].fxaaColorBuffer;
        }

        auto toggleDisplayEffect() { applyEffect = !applyEffect; }
        auto toggleGammaCorrection() { applyGammaCorrection = !applyGammaCorrection; }

    private:
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};

        struct PostProcessingParams {
            ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            shared_ptr<vireo::DescriptorSet>    fxaaDescriptorSet;
            shared_ptr<vireo::RenderTarget>     fxaaColorBuffer;
            shared_ptr<vireo::DescriptorSet>    effectDescriptorSet;
            shared_ptr<vireo::RenderTarget>     effectColorBuffer;
            shared_ptr<vireo::DescriptorSet>    gammaCorrectionDescriptorSet;
            shared_ptr<vireo::RenderTarget>     gammaCorrectionColorBuffer;
        };

        static constexpr vireo::DescriptorIndex BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex BINDING_INPUT{1};

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}}
        };

        bool                                applyEffect{false};
        bool                                applyGammaCorrection{false};
        shared_ptr<vireo::Vireo>            vireo;
        vector<FrameData>                   framesData;
        PostProcessingParams                params{};
        shared_ptr<vireo::Buffer>           paramsBuffer;
        shared_ptr<vireo::Pipeline>         fxaaPipeline;
        shared_ptr<vireo::Pipeline>         effectPipeline;
        shared_ptr<vireo::Pipeline>         gammaCorrectionPipeline;
        shared_ptr<vireo::Sampler>          sampler;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;

        static float getCurrentTimeMilliseconds();
    };

}