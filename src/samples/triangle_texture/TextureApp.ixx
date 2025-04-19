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
        static constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};

        const vireo::DescriptorIndex BINDING_TEXTURE{0};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        struct Vertex {
            vec3 pos;
            vec2 uv;
        };
        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"TEXCOORD", vireo::AttributeFormat::R32G32_FLOAT, 12},
        };

        vector<Vertex> triangleVertices{
                { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.0f } },
                { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f } },
                { { -0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f } }
        };

        shared_ptr<vireo::Buffer>  vertexBuffer;
        shared_ptr<vireo::Image>   texture;
        shared_ptr<vireo::Sampler> sampler;

        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
            shared_ptr<vireo::Fence>            inFlightFence;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        static constexpr auto defaultPipelineConfig = vireo::GraphicPipeline::Configuration {};

        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;
        shared_ptr<vireo::Pipeline>         defaultPipeline;
        shared_ptr<vireo::SwapChain>        swapChain;

        static vector<unsigned char> generateTextureData(uint32_t width, uint32_t height);
    };
}