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
            const std::shared_ptr<vireo::Vireo>& vireo,
            vireo::ImageFormat renderFormat,
            uint32_t framesInFlight);
        void onKeyDown(KeyScanCodes keyCode);
        void onResize(const vireo::Extent& extent);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);

        auto getColorBuffer(const uint32_t frameIndex) const {
            return applyGammaCorrection ? framesData[frameIndex].gammaCorrectionColorBuffer :
                   applyEffect ? framesData[frameIndex].effectColorBuffer :
                   applyFXAA ? framesData[frameIndex].fxaaColorBuffer:
                   nullptr;
        }

        auto toggleDisplayEffect() { applyEffect = !applyEffect; }
        auto toggleGammaCorrection() { applyGammaCorrection = !applyGammaCorrection; }
        auto toggleFXAA() { applyFXAA = !applyFXAA; }
        auto toggleSMAA() { applySMAA = !applySMAA; }

    private:
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};

        struct PostProcessingParams {
            glm::ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            std::shared_ptr<vireo::DescriptorSet> fxaaDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  fxaaColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> effectDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  effectColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> gammaCorrectionDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  gammaCorrectionColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> smaaDescriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex BINDING_INPUT{1};

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}}
        };

        bool                                     applySMAA{true};
        bool                                     applyFXAA{false};
        bool                                     applyEffect{false};
        bool                                     applyGammaCorrection{false};
        std::shared_ptr<vireo::Vireo>            vireo;
        std::vector<FrameData>                   framesData;
        PostProcessingParams                     params{};
        std::shared_ptr<vireo::Buffer>           paramsBuffer;
        std::shared_ptr<vireo::Pipeline>         fxaaPipeline;
        std::shared_ptr<vireo::Pipeline>         effectPipeline;
        std::shared_ptr<vireo::Pipeline>         gammaCorrectionPipeline;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;

        static float getCurrentTimeMilliseconds();
    };

}