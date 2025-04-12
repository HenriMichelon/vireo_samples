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

    class HelloTriangleApp final : public Application {
    public:
        void onInit() override;
        void onUpdate() override;
        void onRender() override;
        void onDestroy() override;

        void onKeyDown(uint32_t /*key*/) override {}
        void onKeyUp(uint32_t /*key*/) override {}

    protected:
        static constexpr auto TextureWidth = 256;
        static constexpr auto TextureHeight = 256;
        static constexpr auto TexturePixelSize = 4;
        std::vector<unsigned char> generateTextureData() const;

    private:
        struct Vertex {
            glm::vec3 pos;
            glm::vec2 uv;
            glm::vec3 color;
        };
        struct GlobalUBO1 {
            glm::vec4 offset;
            alignas(16) float scale{1.0f};
        };
        GlobalUBO1 ubo1{};
        struct GlobalUBO2 {
            glm::vec3 color;
        };
        GlobalUBO2 ubo2{};

        const backend::DescriptorIndex BINDING_UBO1{0};
        const backend::DescriptorIndex BINDING_UBO2{1};
        const backend::DescriptorIndex BINDING_TEXTURE{2};

        const backend::DescriptorIndex BINDING_SAMPLERS{0};

        const std::vector<backend::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", backend::VertexInputLayout::R32G32B32_FLOAT, 0},
            {"TEXCOORD",    backend::VertexInputLayout::R32G32_FLOAT, 12},
            {"COLOR",    backend::VertexInputLayout::R32G32B32_FLOAT, 20}
        };


        float colorIncrement{1.0f};
        float scaleIncrement{1.0f};

        std::vector<std::shared_ptr<backend::FrameData>> framesData{backend::SwapChain::FRAMES_IN_FLIGHT};
        std::vector<std::shared_ptr<backend::CommandAllocator>> graphicCommandAllocator{backend::SwapChain::FRAMES_IN_FLIGHT};
        std::vector<std::shared_ptr<backend::CommandList>> graphicCommandList{backend::SwapChain::FRAMES_IN_FLIGHT};
        std::shared_ptr<backend::DescriptorLayout> descriptorLayout;
        std::shared_ptr<backend::DescriptorLayout> samplersDescriptorLayout;
        std::map<std::string, std::shared_ptr<backend::PipelineResources>> pipelineResources;
        std::map<std::string, std::shared_ptr<backend::Pipeline>> pipelines;
        std::shared_ptr<backend::Buffer> vertexBuffer;
        std::shared_ptr<backend::Buffer> uboBuffer1;
        std::shared_ptr<backend::Buffer> uboBuffer2;
        std::vector<std::shared_ptr<backend::Image>> textures;
        std::vector<std::shared_ptr<backend::Sampler>> samplers;

    };
}