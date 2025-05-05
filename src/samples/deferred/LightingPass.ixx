/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred.lightingpass;

import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;
import samples.deferred.gbuffer;

export namespace samples {
    class LightingPass {
    public:
        void onInit(
           const std::shared_ptr<vireo::Vireo>& vireo,
           vireo::ImageFormat renderFormat,
           const Scene& scene,
           const DepthPrepass& depthPrepass,
           uint32_t framesInFlight);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            const GBufferPass& gBufferPass,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);
        void onDestroy();

    private:
        struct FrameData {
            std::shared_ptr<vireo::Buffer>        globalUniform;
            std::shared_ptr<vireo::Buffer>        lightUniform;
            std::shared_ptr<vireo::DescriptorSet> descriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{1};
        static constexpr vireo::DescriptorIndex BINDING_POSITION_BUFFER{2};
        static constexpr vireo::DescriptorIndex BINDING_NORMAL_BUFFER{3};
        static constexpr vireo::DescriptorIndex BINDING_ALBEDO_BUFFER{4};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL_BUFFER{5};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}},
            .stencilTestEnable = true,
            .frontStencilOpState = {
                .failOp = vireo::StencilOp::KEEP,
                .passOp = vireo::StencilOp::KEEP,
                .depthFailOp = vireo::StencilOp::KEEP,
                .compareOp = vireo::CompareOp::EQUAL,
                .compareMask = 0xff,
                .writeMask = 0x00
            }
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}},
            .stencilTestEnable = pipelineConfig.stencilTestEnable,
        };

        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;
    };
}