/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include <glm/gtc/matrix_transform.hpp>
#include "Libraries.h"
module samples.deferred;

namespace samples {

    void DeferredApp::onUpdate() {
        scene.onUpdate();
        skybox.onUpdate(scene);
        postProcessing.onUpdate();
    }

    void DeferredApp::onKeyDown(const uint32_t key) {
        const auto keyCode = static_cast<Scene::KeyCodes>(key);
        if (keyCode == Scene::KeyCodes::P) {
            applyPostProcessing = !applyPostProcessing;
            return;
        }
        scene.onKeyDown(key);
    }

    void DeferredApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"MainQueue");
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats[0],
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();

        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());
        skybox.onInit(vireo, uploadCommandList, graphicQueue, swapChain->getFramesInFlight());

        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Scene::Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Scene::Model));
            frame.modelBuffer->map();

            frame.globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Scene::Global));
            frame.globalBuffer->map();

            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true, L"InFlightFence");

            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelBuffer);
        }

        depthPrepass.onInit(vireo, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, swapChain->getFramesInFlight());

        transferQueue->waitIdle();
    }

    void DeferredApp::onRender() {
        const auto frameIndex = swapChain->getCurrentFrameIndex();
        const auto& frame = framesData[frameIndex];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        frame.modelBuffer->write(&scene.getModel());
        frame.globalBuffer->write(&scene.getGlobal());

        depthPrepass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            graphicQueue);

        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            depthPrepass,
            frame.colorBuffer,
            graphicQueue);

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();
        renderingConfig.colorRenderTargets[0].renderTarget = frame.colorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(swapChain->getExtent());
        cmdList->setScissors(swapChain->getExtent());
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet});
        scene.draw(cmdList);
        cmdList->endRendering();
        cmdList->barrier(
            depthPrepass.getDepthBuffer(frameIndex),
            vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL_READ,
            vireo::ResourceState::UNDEFINED);

        shared_ptr<vireo::RenderTarget> colorRenderTarget;
        if (applyPostProcessing) {
            colorRenderTarget = postProcessing.getColorBuffer(frameIndex);
            postProcessing.onRender(
                frameIndex,
                swapChain->getExtent(),
                cmdList,
                frame.colorBuffer);
        } else {
            colorRenderTarget = frame.colorBuffer;
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
        }

        cmdList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        cmdList->copy(colorRenderTarget, swapChain);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);
        cmdList->barrier(
            colorRenderTarget,
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::UNDEFINED);
        cmdList->end();

        graphicQueue->submit(
            skybox.getSemaphore(frameIndex),
            vireo::WaitStage::TRANSFER,
            frame.inFlightFence,
            swapChain,
            {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void DeferredApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearValue);
        }
        depthPrepass.onResize(extent);
        skybox.onResize(extent);
        postProcessing.onResize(extent);
    }

    void DeferredApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        for (const auto& frame : framesData) {
            frame.modelBuffer->unmap();
            frame.globalBuffer->unmap();
        }
        depthPrepass.onDestroy();
        skybox.onDestroy();
    }


}