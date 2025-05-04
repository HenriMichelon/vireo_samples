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

    class TextureBufferApp : public Application {
    public:
        void onInit() override;
        void onUpdate() override;
        void onRender() override;
        void onDestroy() override;

    private:
        const vireo::DescriptorIndex BINDING_UBO{0};
        const vireo::DescriptorIndex BINDING_TEXTURE{1};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        struct Vertex {
            glm::vec3 pos;
            glm::vec2 uv;
            glm::vec3 color;
        };

        struct GlobalUBO {
            glm::vec4 offset;
        };

        struct PushConstants {
            glm::vec3 color;
        };

        struct FrameData {
            std::shared_ptr<vireo::Buffer>           globalUniform;
            std::shared_ptr<vireo::CommandAllocator> commandAllocator;
            std::shared_ptr<vireo::CommandList>      commandList;
            std::shared_ptr<vireo::DescriptorSet>    descriptorSet;
            std::shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
            std::shared_ptr<vireo::Fence>            inFlightFence;
        };

        static constexpr auto pushConstantsDesc = vireo::PushConstantsDesc {
            .stage = vireo::ShaderStage::FRAGMENT,
            .size = sizeof(PushConstants),
        };

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, pos)},
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, offsetof(Vertex, uv)},
            {"COLOR", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Vertex, color)}
        };

        std::vector<Vertex> triangleVertices{
            { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc = {{ .blendEnable = true }}
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f}
            }}
        };

        float                                    colorIncrement{1.0f};
        float                                    scaleIncrement{1.0f};
        GlobalUBO                                globalUbo{};
        PushConstants                            pushConstants{};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Buffer>           vertexBuffer;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;
        std::shared_ptr<vireo::SubmitQueue>      graphicSubmitQueue;
        std::shared_ptr<vireo::SwapChain>        swapChain;

        std::vector<std::shared_ptr<vireo::Image>>              textures;
        std::vector<std::shared_ptr<vireo::Sampler>>            samplers;
        std::map<std::string, std::shared_ptr<vireo::Pipeline>> pipelines;

        static void generateTextureData(const std::shared_ptr<vireo::Buffer>&destination, uint32_t width, uint32_t height);
    };
}