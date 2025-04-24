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
            vec3 pos;
            vec2 uv;
            vec3 color;
        };
        struct GlobalUBO {
            vec4 offset;
        };
        struct PushConstants {
            vec3 color;
        };
        static constexpr auto pushConstantsDesc = vireo::PushConstantsDesc {
            .stage = vireo::ShaderStage::FRAGMENT,
            .size = sizeof(PushConstants),
        };

        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, sizeof(vec3)},
            {"COLOR", vireo::AttributeFormat::R32G32B32_FLOAT, sizeof(vec3) + sizeof(vec2)}
        };

        float colorIncrement{1.0f};
        float scaleIncrement{1.0f};

        vector<Vertex> triangleVertices{
            { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };
        GlobalUBO     globalUbo{};
        PushConstants pushConstants{};

        shared_ptr<vireo::Buffer>          vertexBuffer;
        shared_ptr<vireo::Buffer>          globalUboBuffer;
        vector<shared_ptr<vireo::Image>>   textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
            shared_ptr<vireo::Fence>            inFlightFence;
        };

        vector<FrameData>                   framesData;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;
        shared_ptr<vireo::SubmitQueue>      graphicSubmitQueue;
        shared_ptr<vireo::SwapChain>        swapChain;

        static constexpr auto pipelineConfig = vireo::GraphicPipelineConfiguration {
            .colorRenderFormat = vireo::ImageFormat::R8G8B8A8_SRGB,
            .colorBlendDesc = {
                .blendEnable = true,
            }
        };
        vireo::RenderingConfiguration renderingConfig {
            .clearColorValue = {0.0f, 0.2f, 0.4f, 1.0f}
        };
        map<string, shared_ptr<vireo::PipelineResources>> pipelinesResources;
        map<string, shared_ptr<vireo::Pipeline>>          pipelines;

        static vector<unsigned char> generateTextureData(uint32_t width, uint32_t height);
    };
}