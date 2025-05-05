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
            std::shared_ptr<vireo::DescriptorSet> modeDescriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL{1};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{2};
        static constexpr vireo::DescriptorIndex BINDING_TEXTURES{3};
        static constexpr vireo::DescriptorIndex BINDING_MODELS{0};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
                {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
                {"NORMAL",   vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, normal)},
                {"UV",       vireo::AttributeFormat::R32G32_FLOAT,    offsetof(Vertex, uv)},
                {"TANGENT",  vireo::AttributeFormat::R32G32B32_FLOAT,   offsetof(Vertex, tangent)},
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc   = {{}},
            .cullMode         = vireo::CullMode::BACK,
            .depthTestEnable  = true,
            .depthWriteEnable = false,
        };
        vireo::RenderingConfiguration renderingConfig {
             .colorRenderTargets = {{
                .clear      = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f},
            }},
        };

        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         opaquePipeline;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> modelDescriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;
    };
}