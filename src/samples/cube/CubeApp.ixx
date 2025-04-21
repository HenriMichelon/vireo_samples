/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellocube;

import samples.app;

export namespace samples {

    class CubeApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onResize() override;
        void onDestroy() override;
        void onUpdate() override;

    private:
        struct Vertex {
            vec3 pos;
            vec3 color;
        };

        struct Global {
            mat4 projection;
            mat4 view;
        };

        struct Model {
            mat4 transform{1.0f};
        };

        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::RenderTarget>     msaaBuffer;
            shared_ptr<vireo::RenderTarget>     depthBuffer;
            shared_ptr<vireo::RenderTarget>     msaaDepthBuffer;
        };

        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, sizeof(vec3)}
        };

        static constexpr auto cameraPos = vec3(0.0f, 0.0f, 2.0f);
        static constexpr auto cameraTarget = vec3(0.0f, 0.0f, 0.0f);
        static constexpr auto AXIS_X = vec3(1.0f, 0.0f, 0.0f);
        static constexpr auto AXIS_Y = vec3(0.0f, 1.0f, 0.0f);
        static constexpr auto AXIS_Z = vec3(0.0f, 0.0f, 1.0f);
        static constexpr auto up = AXIS_Y;

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};

        static constexpr auto pipelineConfig = vireo::GraphicPipelineConfiguration {
            .colorRenderFormat = vireo::ImageFormat::R8G8B8A8_SRGB,
            .msaa = vireo::MSAA::X8,
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = true,
        };
        vireo::RenderingConfiguration renderingConfig {
            .clearColorValue = {0.0f, 0.2f, 0.4f, 1.0f}
        };

        Global                              global{};
        Model                               model{};
        vector<FrameData>                   framesData;
        shared_ptr<vireo::Buffer>           vertexBuffer;
        shared_ptr<vireo::Buffer>           indexBuffer;
        shared_ptr<vireo::Buffer>           globalBuffer;
        shared_ptr<vireo::Buffer>           modelBuffer;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::SwapChain>        swapChain;
        shared_ptr<vireo::SubmitQueue>      graphicQueue;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;

        vector<Vertex> cubeVertices = {
            { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },

            { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
            { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f } },
            { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
        };

        vector<uint32_t> cubeIndices = {
            // front
            0, 1, 2,  2, 3, 0,
            // right
            1, 5, 6,  6, 2, 1,
            // back
            5, 4, 7,  7, 6, 5,
            // left
            4, 0, 3,  3, 7, 4,
            // top
            3, 2, 6,  6, 7, 3,
            // bottom
            4, 5, 1,  1, 0, 4
        };

    };
}