/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.common.skybox;

import samples.common.global;
import samples.common.depthprepass;
import samples.common.scene;

export namespace samples {

    class Skybox {
    public:
        void onUpdate(const Scene& scene);
        void onInit(
            const shared_ptr<vireo::Vireo>& vireo,
            const shared_ptr<vireo::CommandList>& uploadCommandList,
            vireo::ImageFormat renderFormat,
            vireo::ImageFormat depthFormat,
            uint32_t framesInFlight);
        void onDestroy();
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            const DepthPrepass& depthPrepass,
            const shared_ptr<vireo::RenderTarget>& colorBuffer,
            const shared_ptr<vireo::SubmitQueue>& graphicQueue);

        auto getSemaphore(const uint32_t frameIndex) const { return framesData[frameIndex].semaphore; }
        auto getClearValue() const { return renderingConfig.colorRenderTargets[0].clearValue; }

    private:
        struct FrameData {
            shared_ptr<vireo::Buffer>           globalBuffer;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::Semaphore>        semaphore;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_CUBEMAP{1};
        static constexpr vireo::DescriptorIndex BINDING_SAMPLER{0};

        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0 },
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc = {{}},
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f},
            }},
        };

        Global                              global{};
        vector<FrameData>                   framesData;
        shared_ptr<vireo::Vireo>            vireo;
        shared_ptr<vireo::Buffer>           vertexBuffer;
        shared_ptr<vireo::Image>            cubeMap;
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::Sampler>          sampler;
        shared_ptr<vireo::DescriptorLayout> samplerDescriptorLayout;
        shared_ptr<vireo::DescriptorSet>    samplerDescriptorSet;

        vector<float> cubemapVertices{
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

    };

}