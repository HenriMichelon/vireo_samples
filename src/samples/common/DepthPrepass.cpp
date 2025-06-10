/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.common.depthprepass;

namespace samples {

    void DepthPrepass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const Scene& scene,
        const bool withStencil,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->build();

        modelDescriptorLayout = vireo->createDynamicUniformDescriptorLayout();

        if (withStencil) {
            this->withStencil = true;
            renderingConfig.stencilTestEnable = true;
            pipelineConfig.stencilTestEnable = true;
            pipelineConfig.depthStencilImageFormat = vireo::ImageFormat::D32_SFLOAT_S8_UINT;
            pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        }
        pipelineConfig.resources = vireo->createPipelineResources(
            { descriptorLayout, modelDescriptorLayout }
        );
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/depth_prepass.vert");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalUniform->map();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);

            frame.modelUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model), scene.getModels().size());
            frame.modelUniform->map();
            frame.modelDescriptorSet = vireo->createDescriptorSet(modelDescriptorLayout);
            frame.modelDescriptorSet->update(frame.modelUniform);

            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
        }
    }

    void DepthPrepass::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const std::shared_ptr<vireo::Semaphore>& semaphore,
        const std::shared_ptr<vireo::SubmitQueue>& graphicQueue) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());
        frame.modelUniform->write(scene.getModels().data());

        renderingConfig.depthStencilRenderTarget = frame.depthBuffer;

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(
            renderingConfig.depthStencilRenderTarget,
            vireo::ResourceState::UNDEFINED,
            withStencil ? vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL : vireo::ResourceState::RENDER_TARGET_DEPTH);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(vireo::Viewport{
            .width  = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height)});
        cmdList->setScissors(vireo::Rect{
            .width  = extent.width,
            .height = extent.height});
        cmdList->bindPipeline(pipeline);
        if (withStencil) {
            cmdList->setStencilReference(1);
        }
        cmdList->bindDescriptor(frame.descriptorSet, SET_GLOBAL);
        cmdList->bindDescriptor(frame.modelDescriptorSet, SET_MODELS,
            frame.modelUniform->getInstanceSizeAligned() * Scene::MODEL_OPAQUE);
        scene.drawCube(cmdList);
        cmdList->endRendering();
        cmdList->end();

        graphicQueue->submit(
            vireo::WaitStage::VERTEX_SHADER,
            semaphore,
            {cmdList});
    }

    void DepthPrepass::onResize(const vireo::Extent& extent) {
        for (auto& frame : framesData) {
            frame.depthBuffer = vireo->createRenderTarget(
                pipelineConfig.depthStencilImageFormat,
                extent.width,
                extent.height,
                withStencil ? vireo::RenderTargetType::DEPTH_STENCIL : vireo::RenderTargetType::DEPTH,
                renderingConfig.depthStencilClearValue);
        }
    }

}