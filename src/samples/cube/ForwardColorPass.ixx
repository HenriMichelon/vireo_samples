/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.cube.colorpass;

import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;

export namespace samples {
    class ColorPass {
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
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);
        void onDestroy();

        auto getClearValue() const { return renderingConfig.colorRenderTargets[0].clearValue; }

    private:
        struct FrameData {
            std::shared_ptr<vireo::Buffer>        globalUniform;
            std::shared_ptr<vireo::Buffer>        modelUniform;
            std::shared_ptr<vireo::Buffer>        materialUniform;
            std::shared_ptr<vireo::Buffer>        lightUniform;
            std::shared_ptr<vireo::DescriptorSet> descriptorSet;
            std::shared_ptr<vireo::DescriptorSet> modelsDescriptorSet;
            std::shared_ptr<vireo::DescriptorSet> materialsDescriptorSet;
        };

        static constexpr vireo::DescriptorIndex SET_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{1};
        static constexpr vireo::DescriptorIndex BINDING_TEXTURES{2};

        static constexpr vireo::DescriptorIndex SET_SAMPLERS{1};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        static constexpr vireo::DescriptorIndex SET_MODELS{2};
        static constexpr vireo::DescriptorIndex SET_MATERIALS{3};

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
                {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
                {"NORMAL",   vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, normal)},
                {"UV",       vireo::AttributeFormat::R32G32_FLOAT,    offsetof(Vertex, uv)},
                {"TANGENT",  vireo::AttributeFormat::R32G32B32_FLOAT,   offsetof(Vertex, tangent)},
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc   = { { .blendEnable = true } },
            .cullMode         = vireo::CullMode::BACK,
            .depthTestEnable  = true,
            .depthWriteEnable = false,
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear = false,
            }},
            .depthTestEnable     = pipelineConfig.depthTestEnable,
            .discardDepthStencilAfterRender = true,
        };

        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> modelsDescriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> materialsDescriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;
    };
}