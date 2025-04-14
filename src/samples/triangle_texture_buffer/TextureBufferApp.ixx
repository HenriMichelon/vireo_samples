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
        void onUpdate() override;
        void onRender() override;
        void onDestroy() override;

    private:
        const vireo::DescriptorIndex BINDING_UBO1{0};
        const vireo::DescriptorIndex BINDING_UBO2{1};
        const vireo::DescriptorIndex BINDING_TEXTURE{2};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        struct Vertex {
            vec3 pos;
            vec2 uv;
            vec3 color;
        };
        struct GlobalUBO1 {
            vec4 offset;
            alignas(16) float scale{1.0f};
        };
        struct GlobalUBO2 {
            alignas(16) vec3 color;
        };

        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, 12},
            {"COLOR", vireo::AttributeFormat::R32G32B32_FLOAT, 20}
        };

        float colorIncrement{1.0f};
        float scaleIncrement{1.0f};

        vector<Vertex>            triangleVertices;
        GlobalUBO1                ubo1{};
        GlobalUBO2                ubo2{};
        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> uboBuffer1;
        shared_ptr<vireo::Buffer> uboBuffer2;

        vector<shared_ptr<vireo::Image>>   textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

        struct FrameData {
            shared_ptr<vireo::FrameData>        frameData;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        shared_ptr<vireo::DescriptorLayout>      descriptorLayout;
        shared_ptr<vireo::DescriptorLayout>      samplersDescriptorLayout;

        static constexpr auto pipelineConfig = vireo::Pipeline::Configuration {};
        map<string, shared_ptr<vireo::Pipeline>> pipelines;

        static vector<unsigned char> generateTextureData(uint32_t width, uint32_t height);
    };
}