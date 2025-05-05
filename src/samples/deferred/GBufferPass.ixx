/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred.gbuffer;

import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;

export namespace samples {
    class GBufferPass {
    public:
        void onInit(
            const std::shared_ptr<vireo::Vireo>& vireo,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            uint32_t framesInFlight);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            const std::shared_ptr<vireo::CommandList>& cmdList);
        void onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList);
        void onDestroy();

        auto getPositionBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].positionBuffer; }
        auto getNormalBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].normalBuffer; }
        auto getAlbedoBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].albedoBuffer; }
        auto getMaterialBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].materialBuffer; }

    private:
        struct FrameData {
            std::shared_ptr<vireo::Buffer>        globalUniform;
            std::shared_ptr<vireo::Buffer>        modelUniform;
            std::shared_ptr<vireo::Buffer>        materialUniform;
            std::shared_ptr<vireo::DescriptorSet> descriptorSet;
            std::shared_ptr<vireo::RenderTarget>  positionBuffer;
            std::shared_ptr<vireo::RenderTarget>  normalBuffer;
            std::shared_ptr<vireo::RenderTarget>  albedoBuffer;
            std::shared_ptr<vireo::RenderTarget>  materialBuffer;
        };

        struct PushConstants {
            uint32_t modelIndex;
            uint32_t materialIndex;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL{2};
        static constexpr vireo::DescriptorIndex BINDING_TEXTURES{3};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        static constexpr int BUFFER_POSITION{0};
        static constexpr int BUFFER_NORMAL{1};
        static constexpr int BUFFER_ALBEDO{2};
        static constexpr int BUFFER_MATERIAL{3};

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
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats  = {
                vireo::ImageFormat::R16G16B16A16_SFLOAT, // Position
                vireo::ImageFormat::R16G16B16A16_SFLOAT, // Normal
                vireo::ImageFormat::R8G8B8A8_UNORM,      // Albedo
                vireo::ImageFormat::R8G8_UNORM,          // Shininess/AO
            },
            .colorBlendDesc      = { {}, {}, {}, {} },
            .cullMode            = vireo::CullMode::BACK,
            .depthTestEnable     = true,
            .depthWriteEnable    = false,
            .stencilTestEnable   = true,
            .frontStencilOpState = {
                .failOp      = vireo::StencilOp::KEEP,
                .passOp      = vireo::StencilOp::KEEP,
                .depthFailOp = vireo::StencilOp::KEEP,
                .compareOp   = vireo::CompareOp::EQUAL,
                .compareMask = 0xff,
                .writeMask   = 0x00
            }
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {
                { .clear = true }, // Position
                { .clear = true }, // Normal
                { .clear = true }, // Albedo
                { .clear = true }, // Shininess/AO
            },
            .depthTestEnable    = pipelineConfig.depthTestEnable,
            .stencilTestEnable  = pipelineConfig.stencilTestEnable,
        };

        PushConstants                            pushConstants{};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;
    };
}