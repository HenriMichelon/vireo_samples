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
        modelDescriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::UNIFORM_DYNAMIC);
        modelDescriptorLayout->build();

        if (withStencil) {
            this->withStencil = true;
            pipelineConfig.stencilTestEnable = true;
            pipelineConfig.depthImageFormat = vireo::ImageFormat::D32_SFLOAT_S8_UINT;
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
            frame.modelDescriptorSet->update(BINDING_MODEL, frame.modelUniform);

            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.semaphore = vireo->createSemaphore(vireo::SemaphoreType::BINARY);
        }
    }

    void DepthPrepass::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const std::shared_ptr<vireo::SubmitQueue>& graphicQueue) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());
        frame.modelUniform->write(scene.getModels().data());

        renderingConfig.depthRenderTarget = frame.depthBuffer;

        frame.commandAllocator->reset();
        auto cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(
            renderingConfig.depthRenderTarget,
            vireo::ResourceState::UNDEFINED,
            withStencil ? vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL : vireo::ResourceState::RENDER_TARGET_DEPTH);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        if (withStencil) {
            cmdList->setStencilReference(1);
        }
        cmdList->setDescriptors({frame.descriptorSet, frame.modelDescriptorSet});
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptor(pipeline, frame.descriptorSet, 0);
        cmdList->bindDescriptor(pipeline, frame.modelDescriptorSet, 1, {
            frame.modelUniform->getInstanceSizeAligned() * Scene::MODEL_OPAQUE,
        });
        scene.drawCube(cmdList);
        cmdList->bindDescriptor(pipeline, frame.modelDescriptorSet, 1, {
            frame.modelUniform->getInstanceSizeAligned() * Scene::MODEL_TRANSPARENT,
        });
        scene.drawCube(cmdList);
        cmdList->endRendering();
        cmdList->end();
        graphicQueue->submit(
            vireo::WaitStage::DEPTH_STENCIL_TEST_BEFORE_FRAGMENT_SHADER,
            frame.semaphore,
            {cmdList});
    }

    void DepthPrepass::onResize(const vireo::Extent& extent) {
        for (auto& frame : framesData) {
            frame.depthBuffer = vireo->createRenderTarget(
                pipelineConfig.depthImageFormat,
                extent.width,
                extent.height,
                withStencil ? vireo::RenderTargetType::DEPTH_STENCIL : vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
        }
    }

    void DepthPrepass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.modelUniform->unmap();
            frame.globalUniform->unmap();
        }
    }

}