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
            const shared_ptr<vireo::Vireo>& vireo,
            const Scene& scene,
            vireo::ImageFormat depthImageFormat,
            uint32_t framesInFlight);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            const shared_ptr<vireo::CommandList>& cmdList);
        void onResize(const vireo::Extent& extent, const shared_ptr<vireo::CommandList>& cmdList);
        void onDestroy();

        auto getPositionBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].positionBuffer; }
        auto getNormalBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].normalBuffer; }
        auto getAlbedoBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].albedoBuffer; }
        auto getMaterialBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].materialBuffer; }

    private:
        struct FrameData {
            shared_ptr<vireo::Buffer>        globalUniform;
            shared_ptr<vireo::Buffer>        modelUniform;
            shared_ptr<vireo::Buffer>        materialUniform;
            shared_ptr<vireo::DescriptorSet> descriptorSet;
            shared_ptr<vireo::RenderTarget>  positionBuffer;
            shared_ptr<vireo::RenderTarget>  normalBuffer;
            shared_ptr<vireo::RenderTarget>  albedoBuffer;
            shared_ptr<vireo::RenderTarget>  materialBuffer;
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

        const vector<vireo::VertexAttributeDesc> vertexAttributes {
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
            {"NORMAL",   vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, normal)},
            {"UV",       vireo::AttributeFormat::R32G32_FLOAT,    offsetof(Vertex, uv)},
            {"TANGENT",  vireo::AttributeFormat::R32G32B32_FLOAT,   offsetof(Vertex, tangent)},
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {
                vireo::ImageFormat::R16G16B16A16_SFLOAT, // Position
                vireo::ImageFormat::R16G16B16A16_SFLOAT, // Normal
                vireo::ImageFormat::R8G8B8A8_UNORM,      // Albedo
                vireo::ImageFormat::R8G8_UNORM,          // Shininess/AO
            },
            .colorBlendDesc = { {}, {}, {}, {} },
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = true,
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
            .colorRenderTargets = {
                { .clear = true }, // Position
                { .clear = true }, // Normal
                { .clear = true }, // Albedo
                { .clear = true }, // Shininess/AO
            },
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