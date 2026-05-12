/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
export module samples.common.postprocessing;

import glm;
import std;
import vireo;
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
            std::uint32_t framesInFlight);

        void onResize(const vireo::Extent& extent);

        void onRender(
            std::uint32_t frameIndex,
            const vireo::Extent& extent,
            const Samplers& samplers,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);

        std::shared_ptr<vireo::QueryPool> taaPass(
            std::uint32_t frameIndex,
            const vireo::Extent& extent,
            const Samplers& samplers,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer,
            const std::shared_ptr<vireo::RenderTarget>& velocityBuffer);

        auto getColorBuffer(const std::uint32_t frameIndex) const {
            return
                   applyFXAA ? framesData[frameIndex].fxaaColorBuffer:
                   applySMAA ? framesData[frameIndex].smaaColorBuffer:
                   applyGammaCorrection ? framesData[frameIndex].gammaCorrectionColorBuffer :
                   applyEffect ? framesData[frameIndex].effectColorBuffer :
                   applyTAA ? framesData[frameIndex].taaColorBuffer[taaIndex]:
                   nullptr;
        }

        bool applyTAA{false};

        auto getTAAColorBuffer(const std::uint32_t frameIndex) const {
            return framesData[frameIndex].taaColorBuffer[taaIndex];
        }

    private:
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};
        static constexpr vireo::DescriptorIndex BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex BINDING_INPUT{1};
        static constexpr vireo::DescriptorIndex BINDING_HISTORY{2}; // TAA Only
        static constexpr vireo::DescriptorIndex BINDING_VELOCITY{3}; // TAA Only

        // SMAA compute descriptor bindings (space 0)
        static constexpr vireo::DescriptorIndex SMAA_BINDING_DATA{1};
        static constexpr vireo::DescriptorIndex SMAA_BINDING_OUTPUT{2};
        static constexpr vireo::DescriptorIndex SMAA_BINDING_TEXTURES{3};
        // SMAA compute descriptor bindings (space 2)
        static constexpr vireo::DescriptorIndex SMAA_BINDING_EDGE{0};
        static constexpr vireo::DescriptorIndex SMAA_BINDING_WEIGHT{1};

        static constexpr uint32_t TILE_SIZE{16};
        static constexpr int SMAA_TEXTURES_COUNT{3};

        struct PostProcessingParams {
            glm::ivec2 imageSize{};
            float time;
        };

        struct SmaaData {
            float edgeThreshold{0.1f};
            int   blendMaxSteps{16};
        };

        struct FrameData {
            std::shared_ptr<vireo::DescriptorSet> fxaaDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  fxaaColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> effectDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  effectColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> gammaCorrectionDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  gammaCorrectionColorBuffer;
            std::shared_ptr<vireo::DescriptorSet> smaaComputeDescriptorSet;
            std::shared_ptr<vireo::DescriptorSet> smaaExtraDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  smaaColorBuffer;
            std::shared_ptr<vireo::Image>         smaaColorImage;
            std::shared_ptr<vireo::Image>         smaaEdgeBuffer;
            std::shared_ptr<vireo::Image>         smaaBlendBuffer;
            std::shared_ptr<vireo::DescriptorSet> taaDescriptorSet[2];
            std::shared_ptr<vireo::RenderTarget>  taaColorBuffer[2];
            bool colorBuffersInitialized{false};
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

        std::shared_ptr<vireo::Vireo>             vireo;
        std::vector<FrameData>                    framesData;
        PostProcessingParams                      params{};
        SmaaData                                  smaaData{};
        std::shared_ptr<vireo::Buffer>            paramsBuffer;
        std::shared_ptr<vireo::Buffer>            smaaDataBuffer;
        std::shared_ptr<vireo::Pipeline>          smaaEdgePipeline;
        std::shared_ptr<vireo::Pipeline>          smaaBlendWeightPipeline;
        std::shared_ptr<vireo::Pipeline>          smaaBlendPipeline;
        std::shared_ptr<vireo::Pipeline>          taaPipeline;
        std::shared_ptr<vireo::Pipeline>          fxaaPipeline;
        std::shared_ptr<vireo::Pipeline>          effectPipeline;
        std::shared_ptr<vireo::Pipeline>          gammaCorrectionPipeline;
        std::shared_ptr<vireo::DescriptorLayout>  descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout>  taaDescriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout>  smaaComputeDescLayout;
        std::shared_ptr<vireo::DescriptorLayout>  smaaExtraDescLayout;
        std::shared_ptr<vireo::PipelineResources> smaaComputeResources;

        std::uint32_t taaIndex{0};

        static float getCurrentTimeMilliseconds();

        auto toggleDisplayEffect() { applyEffect = !applyEffect; }
        auto toggleGammaCorrection() { applyGammaCorrection = !applyGammaCorrection; }
        auto toggleFXAA() { applyFXAA = !applyFXAA; }
        auto toggleSMAA() { applySMAA = !applySMAA; }
        auto toggleTAA() { applyTAA = !applyTAA; }

    };

}