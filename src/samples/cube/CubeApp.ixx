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
        void onKeyDown(uint32_t key) override;

    private:
#ifdef _WIN32
        enum class KeyCodes : uint32_t {
           LEFT     = 37,
           UP       = 38,
           RIGHT    = 39,
           DOWN     = 40,
           P        = 80,
        };
#endif

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

        struct PostProcessingParams {
            ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::RenderTarget>     colorBuffer;
            shared_ptr<vireo::RenderTarget>     msaaColorBuffer;
            shared_ptr<vireo::RenderTarget>     depthBuffer;
            shared_ptr<vireo::RenderTarget>     msaaDepthBuffer;

            shared_ptr<vireo::RenderTarget>     postProcessingColorBuffer;
            shared_ptr<vireo::DescriptorSet>    postProcessingDescriptorSet;
        };

        static constexpr auto AXIS_X = vec3(1.0f, 0.0f, 0.0f);
        static constexpr auto AXIS_Y = vec3(0.0f, 1.0f, 0.0f);
        static constexpr auto AXIS_Z = vec3(0.0f, 0.0f, 1.0f);
        static constexpr auto up = AXIS_Y;

        // Global data
        bool                           applyPostProcessing{true};
        float                          cameraYRotationAngle{0.0f};
        vec3                           cameraPos{0.0f, 0.0f, 2.0f};
        vec3                           cameraTarget{0.0f, 0.0f, 0.0f};
        vector<FrameData>              framesData;
        shared_ptr<vireo::SwapChain>   swapChain;
        shared_ptr<vireo::SubmitQueue> graphicQueue;

        // Cube rendering data
        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, sizeof(vec3)}
        };
        const vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc = {{}},
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
        shared_ptr<vireo::Buffer>           vertexBuffer;
        shared_ptr<vireo::Buffer>           indexBuffer;
        shared_ptr<vireo::Buffer>           globalBuffer;
        shared_ptr<vireo::Buffer>           modelBuffer;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;

        // Skybox rendering data
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_CUBEMAP{1};
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_SAMPLER{0};
        const vireo::GraphicPipelineConfiguration skyboxPipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc = {{}},
            .msaa = vireo::MSAA::X8,
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
        };
        const vector<vireo::VertexAttributeDesc> skyboxVertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
        };
        Global                              skyboxGlobal{};
        shared_ptr<vireo::Sampler>          skyboxSampler;
        shared_ptr<vireo::Buffer>           skyboxGlobalBuffer;
        shared_ptr<vireo::Buffer>           skyboxVertexBuffer;
        shared_ptr<vireo::Image>            skyboxCubeMap;
        shared_ptr<vireo::Pipeline>         skyboxPipeline;
        shared_ptr<vireo::DescriptorLayout> skyboxDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    skyboxDescriptorSet;
        shared_ptr<vireo::DescriptorLayout> skyboxSamplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    skyboxSamplerDescriptorSet;

        // Post-processing data
        static constexpr vireo::DescriptorIndex POSTPROCESSING_BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex POSTPROCESSING_BINDING_INPUT{1};
        const vireo::GraphicPipelineConfiguration postprocessingPipelineConfig {
            .colorRenderFormats = { vireo::ImageFormat::R8G8B8A8_SRGB },
            .colorBlendDesc = {{}}
        };
        const vector<vireo::VertexAttributeDesc> postprocessingAttributes{};
        vireo::RenderingConfiguration postprocessingRenderingConfig {
            .clearColor = false
        };
        PostProcessingParams                postprocessingParams{};
        shared_ptr<vireo::Buffer>           postprocessingParamsBuffer;
        shared_ptr<vireo::Pipeline>         postprocessingPipeline;
        shared_ptr<vireo::DescriptorLayout> postprocessingDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    postprocessingDescriptorSet;

        // Models data
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

        vector<float> skyboxVertices{
            -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
             1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

             -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
             -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

             1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

             -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

             -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

             -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
             1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

        shared_ptr<vireo::Image> loadCubemap(
            const shared_ptr<vireo::CommandList>& cmdList,
            const string &filepath,
            vireo::ImageFormat imageFormat) const;

        static std::byte* loadRGBAImage(const string& filepath, uint32_t& width, uint32_t& height, uint64_t& size);

        static std::byte *extractImage(
            const std::byte *source,
            int   x, int y,
            int   srcWidth,
            int   w,  int h,
            int   channels);

        static float getCurrentTimeMilliseconds();

    };
}