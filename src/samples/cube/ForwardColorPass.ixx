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
           const shared_ptr<vireo::Vireo>& vireo,
           vireo::ImageFormat renderFormat,
           const Scene& scene,
           uint32_t framesInFlight);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const DepthPrepass& depthPrepass,
            const shared_ptr<vireo::CommandList>& cmdList,
            const shared_ptr<vireo::RenderTarget>& colorBuffer);
        void onDestroy();

    private:
        struct FrameData {
            shared_ptr<vireo::Buffer>        globalBuffer;
            shared_ptr<vireo::Buffer>        modelBuffer;
            shared_ptr<vireo::Buffer>        materialBuffer;
            shared_ptr<vireo::Buffer>        lightBuffer;
            shared_ptr<vireo::DescriptorSet> descriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        static constexpr vireo::DescriptorIndex BINDING_MATERIAL{2};
        static constexpr vireo::DescriptorIndex BINDING_LIGHT{3};
        static constexpr vireo::DescriptorIndex BINDING_TEXTURES{4};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLERS{0};

        const vector<vireo::VertexAttributeDesc> vertexAttributes{
                {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
                {"NORMAL",   vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, normal)},
                {"UV",       vireo::AttributeFormat::R32G32_FLOAT,    offsetof(Vertex, uv)},
                {"TANGENT",  vireo::AttributeFormat::R32G32B32_FLOAT,   offsetof(Vertex, tangent)},
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}},
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
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