/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred.oitpass;

import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;
import samples.common.samplers;
import samples.deferred.gbuffer;

export namespace samples {
    class TransparencyPass {
    public:
        void onInit(
           const std::shared_ptr<vireo::Vireo>& vireo,
           vireo::ImageFormat renderFormat,
           const Scene& scene,
           const DepthPrepass& depthPrepass,
           const Samplers& samplers,
           uint32_t framesInFlight);
        void onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            const Samplers& samplers,
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer);
        void onDestroy();

    private:
        struct FrameData {
            std::shared_ptr<vireo::Buffer>        globalUniform;
            std::shared_ptr<vireo::Buffer>        modelUniform;
            std::shared_ptr<vireo::Buffer>        lightUniform;
            std::shared_ptr<vireo::Buffer>        materialUniform;
            std::shared_ptr<vireo::DescriptorSet> oitDescriptorSet;
            std::shared_ptr<vireo::DescriptorSet> compositeDescriptorSet;
            std::shared_ptr<vireo::RenderTarget>  accumBuffer;
            std::shared_ptr<vireo::RenderTarget>  revealageBuffer;
        };

        struct PushConstants {
            uint32_t modelIndex;
            uint32_t materialIndex;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{2};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL{3};
        static constexpr vireo::DescriptorIndex BINDING_TEXTURES{4};

        static constexpr vireo::DescriptorIndex BINDING_ACCUM_BUFFER{0};
        static constexpr vireo::DescriptorIndex BINDING_REVEALAGE_BUFFER{1};

        static constexpr auto pushConstantsDesc = vireo::PushConstantsDesc {
            .stage = vireo::ShaderStage::ALL,
            .size = sizeof(PushConstants),
        };
        const std::vector<vireo::VertexAttributeDesc> vertexAttributes {
                {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
                {"NORMAL",   vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, normal)},
                {"UV",       vireo::AttributeFormat::R32G32_FLOAT,    offsetof(Vertex, uv)},
                {"TANGENT",  vireo::AttributeFormat::R32G32B32_FLOAT,   offsetof(Vertex, tangent)},
            };

        vireo::GraphicPipelineConfiguration oitPipelineConfig {
            .colorRenderFormats  = {
                vireo::ImageFormat::R16G16B16A16_SFLOAT, // Color accumulation
                vireo::ImageFormat::R16_SFLOAT,          // Alpha accumulation
            },
            .colorBlendDesc = {
                {
                    .blendEnable = true,
                    .srcColorBlendFactor = vireo::BlendFactor::ONE,
                    .dstColorBlendFactor = vireo::BlendFactor::ONE,
                    .colorBlendOp = vireo::BlendOp::ADD,
                    .srcAlphaBlendFactor = vireo::BlendFactor::ONE,
                    .dstAlphaBlendFactor = vireo::BlendFactor::ONE,
                    .alphaBlendOp = vireo::BlendOp::ADD,
                    .colorWriteMask = vireo::ColorWriteMask::ALL,
                },
                {
                    .blendEnable = true,
                    .srcColorBlendFactor = vireo::BlendFactor::ZERO,
                    .dstColorBlendFactor = vireo::BlendFactor::ONE_MINUS_SRC_COLOR,
                    .colorBlendOp = vireo::BlendOp::ADD,
                    .srcAlphaBlendFactor = vireo::BlendFactor::ONE,
                    .dstAlphaBlendFactor = vireo::BlendFactor::ONE,
                    .alphaBlendOp = vireo::BlendOp::ADD,
                    .colorWriteMask = vireo::ColorWriteMask::RED,
                }},
            .depthTestEnable = true,
            .depthWriteEnable = false
        };
        vireo::RenderingConfiguration oitRenderingConfig {
            .colorRenderTargets = {
                {
                    .clear = true,
                    .clearValue = {0.0f, 0.0f, 0.0f, 0.0f},
                },
                {
                    .clear = true,
                    .clearValue = {1.0f, 0.0f, 0.0f, 0.0f},
                }
            },
            .depthTestEnable = oitPipelineConfig.depthTestEnable,
            .discardDepthStencilAfterRender = true,
        };

        vireo::GraphicPipelineConfiguration compositePipelineConfig {
            .colorBlendDesc = {
                {
                        .blendEnable = true,
                        .srcColorBlendFactor = vireo::BlendFactor::ONE_MINUS_SRC_ALPHA,
                        .dstColorBlendFactor = vireo::BlendFactor::SRC_ALPHA,
                        .colorBlendOp = vireo::BlendOp::ADD,
                        .srcAlphaBlendFactor = vireo::BlendFactor::ONE,
                        .dstAlphaBlendFactor = vireo::BlendFactor::ZERO,
                        .alphaBlendOp = vireo::BlendOp::ADD,
                        .colorWriteMask = vireo::ColorWriteMask::ALL,
                }
            },
            .depthTestEnable = false,
            .depthWriteEnable = false
        };
        vireo::RenderingConfiguration compositeRenderingConfig {
            .colorRenderTargets = {{}},
        };

        PushConstants                            pushConstants{};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         oitPipeline;
        std::shared_ptr<vireo::Pipeline>         compositePipeline;
        std::shared_ptr<vireo::DescriptorLayout> oitDescriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> compositeDescriptorLayout;
    };
}