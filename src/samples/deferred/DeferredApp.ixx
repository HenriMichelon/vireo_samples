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
import samples.deferred.skybox;
import samples.deferred.postprocessing;

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

        struct FrameData {
            shared_ptr<vireo::Buffer>           modelBuffer;
            shared_ptr<vireo::Buffer>           globalBuffer;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::Fence>            inFlightFence;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::RenderTarget>     colorBuffer;
        };

        bool                           applyPostProcessing{false};
        vector<FrameData>              framesData;
        shared_ptr<vireo::SwapChain>   swapChain;
        shared_ptr<vireo::SubmitQueue> graphicQueue;
        Scene                          scene;
        DepthPrepass                   depthPrepass;
        Skybox                         skybox;
        PostProcessing                 postProcessing;

        static constexpr vireo::DescriptorIndex BINDING_GLOBAL{0};
        static constexpr vireo::DescriptorIndex BINDING_MODEL{1};
        const vector<vireo::VertexAttributeDesc> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Scene::Vertex, pos) },
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, offsetof(Scene::Vertex, color)}
        };
        vireo::GraphicPipelineConfiguration pipelineConfig {
            .colorRenderFormats = {vireo::ImageFormat::R8G8B8A8_UNORM},
            .colorBlendDesc = {{}},
            .cullMode = vireo::CullMode::BACK,
            .depthTestEnable = true,
            .depthWriteEnable = false,
        };
        vireo::RenderingConfiguration renderingConfig {
            .colorRenderTargets = {{}},
            .discardDepthAfterRender = true,
        };
        shared_ptr<vireo::Pipeline>         pipeline;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
    };
}