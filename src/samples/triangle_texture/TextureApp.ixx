/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellotexture;

import samples.app;

export namespace samples {

    class TextureApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onDestroy() override;

    private:
        const vireo::DescriptorIndex BINDING_TEXTURE{0};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        struct Vertex {
            glm::vec3 pos;
            glm::vec2 uv;
        };
        struct FrameData {
            std::shared_ptr<vireo::CommandAllocator> commandAllocator;
            std::shared_ptr<vireo::CommandList>      commandList;
            std::shared_ptr<vireo::DescriptorSet>    descriptorSet;
            std::shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
            std::shared_ptr<vireo::Fence>            inFlightFence;
        };

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, pos)},
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, offsetof(Vertex, uv)},
        };

        std::vector<Vertex> triangleVertices{
            { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.0f } },
            { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f } }
        };

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc     = {{}},
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear      = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f}
            }}
        };

        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Buffer>           vertexBuffer;
        std::shared_ptr<vireo::Image>            texture;
        std::shared_ptr<vireo::Sampler>          sampler;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::SubmitQueue>      graphicQueue;
        std::shared_ptr<vireo::SwapChain>        swapChain;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;

        static void generateTextureData(const std::shared_ptr<vireo::Buffer>&destination, uint32_t width, uint32_t height);
    };
}