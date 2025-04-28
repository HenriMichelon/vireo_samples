/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellomsaa;

import samples.app;

export namespace samples {

    class MsaaApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onResize() override;
        void onDestroy() override;

    private:
        struct Vertex {
            vec3 pos;
            vec3 color;
        };
        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, pos)},
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, color)}
        };
        vector<Vertex> triangleVertices{
            { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        shared_ptr<vireo::Buffer> vertexBuffer;

        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::RenderTarget>     msaaRenderTarget;
        };
        vector<FrameData> framesData;

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc = {{}},
            .msaa = vireo::MSAA::X8
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f}
            }}
        };

        shared_ptr<vireo::Pipeline>    pipeline;
        shared_ptr<vireo::SwapChain>   swapChain;
        shared_ptr<vireo::SubmitQueue> graphicSubmitQueue;

    };
}