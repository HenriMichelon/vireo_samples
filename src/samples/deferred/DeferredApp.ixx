/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred;

import samples.app;
import samples.deferred.depthprepass;
import samples.deferred.scene;

export namespace samples {

    class DeferredApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onResize() override;
        void onDestroy() override;
        void onUpdate() override;
        void onKeyDown(uint32_t key) override;

    private:
        struct PostProcessingParams {
            ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            shared_ptr<vireo::Buffer>           modelBuffer;
            shared_ptr<vireo::Buffer>           globalBuffer;

            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;

            shared_ptr<vireo::Buffer>           skyboxGlobalBuffer;
            shared_ptr<vireo::DescriptorSet>    skyboxDescriptorSet;

            shared_ptr<vireo::RenderTarget>     colorBuffer;
            shared_ptr<vireo::RenderTarget>     msaaColorBuffer;

            shared_ptr<vireo::RenderTarget>     postProcessingColorBuffer;
            shared_ptr<vireo::DescriptorSet>    postProcessingDescriptorSet;
        };

        // Common data
        bool                           applyPostProcessing{false};
        vector<FrameData>              framesData;
        shared_ptr<vireo::SwapChain>   swapChain;
        shared_ptr<vireo::SubmitQueue> graphicQueue;
        Scene::Global                  skyboxGlobal{};
        Scene                          scene;
        DepthPrepass                   depthPrepass;

        // Cube rendering data
        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Scene::Vertex, pos) },
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Scene::Vertex, color)}
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_SRGB},
            .colorBlendDesc = {{}},
            .msaa = vireo::MSAA::X8,
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f},
            }},
            .discardDepthAfterRender = true,
        };
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;

        // Skybox rendering data
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_CUBEMAP{1};
        static constexpr vireo::DescriptorIndex SKYBOX_BINDING_SAMPLER{0};
        vireo::GraphicPipelineConfiguration skyboxPipelineConfig {
            .colorRenderFormats = {pipelineConfig.colorRenderFormats[0]},
            .colorBlendDesc = {{}},
            .msaa = pipelineConfig.msaa,
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
        };
        const vector<vireo::VertexAttributeDesc> skyboxVertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0 },
        };
        shared_ptr<vireo::Sampler>          skyboxSampler;
        shared_ptr<vireo::Buffer>           skyboxVertexBuffer;
        shared_ptr<vireo::Image>            skyboxCubeMap;
        shared_ptr<vireo::Pipeline>         skyboxPipeline;
        shared_ptr<vireo::DescriptorLayout> skyboxDescriptorLayout;
        shared_ptr<vireo::DescriptorLayout> skyboxSamplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    skyboxSamplerDescriptorSet;

        // Post-processing data
        static constexpr vireo::DescriptorIndex POSTPROCESSING_BINDING_PARAMS{0};
        static constexpr vireo::DescriptorIndex POSTPROCESSING_BINDING_INPUT{1};
        vireo::GraphicPipelineConfiguration postprocessingPipelineConfig {
            .colorRenderFormats = {pipelineConfig.colorRenderFormats[0]},
            .colorBlendDesc = {{}}
        };
        vireo::RenderingConfiguration postprocessingRenderingConfig {
            .colorRenderTargets = {{}}
        };
        PostProcessingParams                postprocessingParams{};
        shared_ptr<vireo::Buffer>           postprocessingParamsBuffer;
        shared_ptr<vireo::Pipeline>         postprocessingPipeline;
        shared_ptr<vireo::DescriptorLayout> postprocessingDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    postprocessingDescriptorSet;

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