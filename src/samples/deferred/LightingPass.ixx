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
           const shared_ptr<vireo::Vireo>& vireo,
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
            const shared_ptr<vireo::CommandList>& cmdList,
            const shared_ptr<vireo::RenderTarget>& colorBuffer);
        void onDestroy();

    private:
        struct FrameData {
            shared_ptr<vireo::Buffer>        globalUniform;
            shared_ptr<vireo::Buffer>        modelUniform;
            shared_ptr<vireo::Buffer>        lightUniform;
            shared_ptr<vireo::DescriptorSet> descriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{2};
        static constexpr vireo::DescriptorIndex BINDING_POSITION_BUFFER{3};
        static constexpr vireo::DescriptorIndex BINDING_NORMAL_BUFFER{4};
        static constexpr vireo::DescriptorIndex BINDING_ALBEDO_BUFFER{5};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL_BUFFER{6};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}},
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
            .discardDepthAfterRender = true,
        };

        vector<FrameData>                   framesData;
        shared_ptr<vireo::Vireo>            vireo;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::Sampler>          sampler;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;
    };
}