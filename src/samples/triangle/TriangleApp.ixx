/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellotriangle;

import samples.app;

export namespace samples {

    class TriangleApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onResize() override;
        void onDestroy() override;

    private:
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 color;
        };

        struct FrameData {
            std::shared_ptr<vireo::CommandAllocator> commandAllocator;
            std::shared_ptr<vireo::CommandList>      commandList;
            std::shared_ptr<vireo::Fence>            inFlightFence;
        };

        std::vector<Vertex> triangleVertices{
            { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, pos)},
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, color)}
        };

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc     = {{}}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear      = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f}
            }}
        };

        std::vector<FrameData>              framesData;
        std::shared_ptr<vireo::Buffer>      vertexBuffer;
        std::shared_ptr<vireo::Pipeline>    defaultPipeline;
        std::shared_ptr<vireo::SwapChain>   swapChain;
        std::shared_ptr<vireo::SubmitQueue> graphicQueue;
    };
}