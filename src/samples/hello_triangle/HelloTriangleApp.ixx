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

    class HelloTriangleApp : public Application {
    public:
        void onInit() override;
        void onUpdate() override;
        void onRender() override;
        void onDestroy() override;

    protected:
        static constexpr auto TextureWidth = 256;
        static constexpr auto TextureHeight = 256;
        static constexpr auto TexturePixelSize = 4;
        vector<unsigned char> generateTextureData() const;

    private:
        struct Vertex {
            vec3 pos;
            vec2 uv;
            vec3 color;
        };
        struct GlobalUBO1 {
            vec4 offset;
            alignas(16) float scale{1.0f};
        };
        GlobalUBO1 ubo1{};
        struct GlobalUBO2 {
            vec3 color;
        };
        GlobalUBO2 ubo2{};

        const vireo::DescriptorIndex BINDING_UBO1{0};
        const vireo::DescriptorIndex BINDING_UBO2{1};
        const vireo::DescriptorIndex BINDING_TEXTURE{2};

        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::VertexInputLayout::R32G32B32_FLOAT, 0},
            {"TEXCOORD",    vireo::VertexInputLayout::R32G32_FLOAT, 12},
            {"COLOR",    vireo::VertexInputLayout::R32G32B32_FLOAT, 20}
        };

        float colorIncrement{1.0f};
        float scaleIncrement{1.0f};

        vector<shared_ptr<vireo::FrameData>> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandAllocator>> graphicCommandAllocator{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandList>> graphicCommandList{vireo::SwapChain::FRAMES_IN_FLIGHT};

        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;

        map<string, shared_ptr<vireo::PipelineResources>> pipelineResources;
        map<string, shared_ptr<vireo::Pipeline>> pipelines;

        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> uboBuffer1;
        shared_ptr<vireo::Buffer> uboBuffer2;
        vector<shared_ptr<vireo::Image>> textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

    };
}