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
import samples.common.samplers;

export namespace samples {

    class Skybox {
    public:
        void onUpdate(const Scene& scene);
        void onInit(
            const std::shared_ptr<vireo::Vireo>& vireo,
            const std::shared_ptr<vireo::CommandList>& uploadCommandList,
            vireo::ImageFormat renderFormat,
            const DepthPrepass& depthPrepass,
            const Samplers& samplers,
            uint32_t framesInFlight);
        void onRender(
            uint32_t frameIndex,
            const vireo::Extent& extent,
            bool depthIsReadOnly,
            const DepthPrepass& depthPrepass,
            const Samplers& samplers,
            const std::shared_ptr<vireo::RenderTarget>& colorBuffer,
            const std::shared_ptr<vireo::Semaphore>& semaphore,
            const std::shared_ptr<vireo::SubmitQueue>& graphicQueue);

        auto getClearValue() const { return renderingConfig.colorRenderTargets[0].clearValue; }

    private:
        struct FrameData : FrameDataCommand {
            std::shared_ptr<vireo::Buffer>        globalBuffer;
            std::shared_ptr<vireo::DescriptorSet> descriptorSet;
        };

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_CUBEMAP{1};

        const std::vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0 },
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorBlendDesc      = {{}},
            .cullMode            = vireo::CullMode::BACK,
            .depthTestEnable     = true,
            .depthWriteEnable    = false,
            .frontStencilOpState = {
                .failOp      = vireo::StencilOp::KEEP,
                .passOp      = vireo::StencilOp::KEEP,
                .depthFailOp = vireo::StencilOp::KEEP,
                .compareOp   = vireo::CompareOp::EQUAL,
                .compareMask = 0xff,
                .writeMask   = 0x00
            }
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{
                .clear      = true,
                .clearValue = {0.0f, 0.2f, 0.4f, 1.0f},
            }},
            .depthTestEnable     = pipelineConfig.depthTestEnable,
            .discardDepthStencilAfterRender = false,
        };

        Global                                   global{};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Vireo>            vireo;
        std::shared_ptr<vireo::Buffer>           vertexBuffer;
        std::shared_ptr<vireo::Image>            cubeMap;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;

        std::vector<float> cubemapVertices{
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

        std::shared_ptr<vireo::Image> loadCubemap(
            const std::shared_ptr<vireo::CommandList>& cmdList,
            const std::string &filepath,
            vireo::ImageFormat imageFormat) const;

        static std::byte* loadRGBAImage(const std::string& filepath, uint32_t& width, uint32_t& height, uint64_t& size);

        static std::byte *extractImage(
            const std::byte *source,
            int   x, int y,
            int   srcWidth,
            int   w,  int h,
            int   channels);
    };

}