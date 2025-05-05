/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.common.depthprepass;

import samples.common.global;
import samples.common.scene;

export namespace samples {

    class DepthPrepass {
    public:
        void onInit(
            const std::shared_ptr<vireo::Vireo>& vireo,
            const Scene& scene,
            bool withStencil,
            uint32_t framesInFlight);
        void onResize(const vireo::Extent& extent);
        void onDestroy();
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const std::shared_ptr<vireo::SubmitQueue>& graphicQueue);

        auto getSemaphore(const uint32_t frameIndex) const { return framesData[frameIndex].semaphore; }
        auto getDepthBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].depthBuffer; }
        auto getFormat() const { return pipelineConfig.depthImageFormat; }
        auto isWithStencil() const { return withStencil; }

    private:
        struct FrameData {
            std::shared_ptr<vireo::Buffer>           globalUniform;
            std::shared_ptr<vireo::Buffer>           modelUniform;
            std::shared_ptr<vireo::CommandAllocator> commandAllocator;
            std::shared_ptr<vireo::CommandList>      commandList;
            std::shared_ptr<vireo::Semaphore>        semaphore;
            std::shared_ptr<vireo::RenderTarget>     depthBuffer;
            std::shared_ptr<vireo::DescriptorSet>    descriptorSet;
            std::shared_ptr<vireo::DescriptorSet>    modelDescriptorSet;
        };

        static constexpr vireo::DescriptorIndex SET_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};

        static constexpr vireo::DescriptorIndex SET_MODELS{1};

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .cullMode            = vireo::CullMode::BACK,
            .depthTestEnable     = true,
            .depthWriteEnable    = true,
            .stencilTestEnable   = false,
            .frontStencilOpState = {
                .failOp      = vireo::StencilOp::KEEP,
                .passOp      = vireo::StencilOp::REPLACE,
                .depthFailOp = vireo::StencilOp::KEEP,
                .compareOp   = vireo::CompareOp::ALWAYS,
                .compareMask = 0xff,
                .writeMask   = 0xff
            }
        };
        vireo::RenderingConfiguration renderingConfig {
            .clearDepth = true,
        };

        bool                                     withStencil{false};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> modelDescriptorLayout;
    };

}