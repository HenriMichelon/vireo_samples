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
        static constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};

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
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, 12},
            {"COLOR", vireo::AttributeFormat::R32G32B32_FLOAT, 20}
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

        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> globalUboBuffer;

        vector<shared_ptr<vireo::Image>>   textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

        struct FrameData {
            shared_ptr<vireo::FrameData>        frameData;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
            shared_ptr<vireo::Fence>            inFlightFence;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        shared_ptr<vireo::DescriptorLayout>      descriptorLayout;
        shared_ptr<vireo::DescriptorLayout>      samplersDescriptorLayout;

        static constexpr auto defaultPipelineConfig = vireo::GraphicPipeline::Configuration {
            .colorBlendEnable = true,
        };
        map<string, shared_ptr<vireo::Pipeline>> pipelines;
        map<string, shared_ptr<vireo::PipelineResources>> pipelinesResources;

        static vector<unsigned char> generateTextureData(uint32_t width, uint32_t height);
    };
}