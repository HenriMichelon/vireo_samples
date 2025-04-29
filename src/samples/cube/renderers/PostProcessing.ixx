/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.cube.postprocessing;

import samples.cube.global;

export namespace samples {

    class PostProcessing {
    public:
        void onUpdate();
        void onInit(
           const shared_ptr<vireo::Vireo>& vireo,
           uint32_t framesInFlight);
        void onResize(const vireo::Extent& extent);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const shared_ptr<vireo::CommandList>& cmdList,
            const shared_ptr<vireo::RenderTarget>& colorBuffer);

        auto getColorBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].colorBuffer; }

    private:
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};

        struct PostProcessingParams {
            ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::RenderTarget>     colorBuffer;
        };

        static constexpr vireo::DescriptorIndex BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex BINDING_INPUT{1};
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {RENDER_FORMAT},
            .colorBlendDesc = {{}}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}}
        };

        shared_ptr<vireo::Vireo>            vireo;
        vector<FrameData>                   framesData;
        PostProcessingParams                params{};
        shared_ptr<vireo::Buffer>           paramsBuffer;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorSet>    descriptorSet;
        shared_ptr<vireo::Sampler>          sampler;
        shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;

        static float getCurrentTimeMilliseconds();

    };

}