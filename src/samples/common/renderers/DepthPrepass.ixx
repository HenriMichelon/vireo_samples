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
        void onInit(const shared_ptr<vireo::Vireo>& vireo,uint32_t framesInFlight);
        void onResize(const vireo::Extent& extent);
        void onDestroy();
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const Scene& scene,
            const shared_ptr<vireo::SubmitQueue>& graphicQueue);

        auto getSemaphore(const uint32_t frameIndex) const { return framesData[frameIndex].semaphore; }
        auto getDepthBuffer(const uint32_t frameIndex) const { return framesData[frameIndex].depthBuffer; }

    private:
        struct FrameData {
            shared_ptr<vireo::Buffer>           modelBuffer;
            shared_ptr<vireo::Buffer>           globalBuffer;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Semaphore>        semaphore;
            shared_ptr<vireo::RenderTarget>     depthBuffer;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};

        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, position) },
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .cullMode         = vireo::CullMode::BACK,
            .depthTestEnable  = true,
            .depthWriteEnable = true,
        };
        vireo::RenderingConfiguration renderingConfig {
            .clearDepth = true,
        };

        vector<FrameData>                   framesData;
        shared_ptr<vireo::Vireo>            vireo;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
    };

}