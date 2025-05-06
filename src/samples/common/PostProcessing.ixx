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
import samples.common.samplers;

export namespace samples {

    class PostProcessing {
    public:
        void onUpdate();
        void onInit(
            const std::shared_ptr<vireo::Vireo>& vireo,
            vireo::ImageFormat renderFormat,
            const Samplers& samplers,
            uint32_t framesInFlight);
        void onKeyDown(KeyScanCodes keyCode);
        void onResize(const vireo::Extent& extent);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Samplers& samplers,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);

        auto getColorBuffer(const uint32_t frameIndex) const {
            return applyGammaCorrection ? framesData[frameIndex].gammaCorrectionColorBuffer :
                   applyEffect ? framesData[frameIndex].effectColorBuffer :
                   applyFXAA ? framesData[frameIndex].fxaaColorBuffer:
                   applySMAA ? framesData[frameIndex].smaaColorBuffer:
                   nullptr;
        }

    private:
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};
        static constexpr vireo::DescriptorIndex BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex BINDING_INPUT{1};
        static constexpr vireo::DescriptorIndex BINDING_SMAA_INPUT{2};

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
            std::shared_ptr<vireo::DescriptorSet> smaaEdgeDescriptorSet;
            std::shared_ptr<vireo::DescriptorSet> smaaBlendWeightDescriptorSet;
            std::shared_ptr<vireo::DescriptorSet> smaaBlendDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  smaaColorBuffer;
            std::shared_ptr<vireo::RenderTarget>  smaaEdgeBuffer;
            std::shared_ptr<vireo::RenderTarget>  smaaBlendBuffer;
        };

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}}
        };

        bool applySMAA{true};
        bool applyFXAA{false};
        bool applyEffect{false};
        bool applyGammaCorrection{true};

        std::shared_ptr<vireo::Vireo>            vireo;
        std::vector<FrameData>                   framesData;
        PostProcessingParams                     params{};
        std::shared_ptr<vireo::Buffer>           paramsBuffer;
        std::shared_ptr<vireo::Pipeline>         smaaEdgePipeline;
        std::shared_ptr<vireo::Pipeline>         smaaBlendWeightPipeline;
        std::shared_ptr<vireo::Pipeline>         smaaBlendPipeline;
        std::shared_ptr<vireo::Pipeline>         fxaaPipeline;
        std::shared_ptr<vireo::Pipeline>         effectPipeline;
        std::shared_ptr<vireo::Pipeline>         gammaCorrectionPipeline;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> smaaDescriptorLayout;

        static float getCurrentTimeMilliseconds();

        auto toggleDisplayEffect() { applyEffect = !applyEffect; }
        auto toggleGammaCorrection() { applyGammaCorrection = !applyGammaCorrection; }
        auto toggleFXAA() { applyFXAA = !applyFXAA; }
        auto toggleSMAA() { applySMAA = !applySMAA; }


    };

}