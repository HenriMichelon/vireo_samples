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
        postprocessingParams.time = getCurrentTimeMilliseconds();
        postprocessingParamsBuffer->write(&postprocessingParams);
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

        postprocessingParamsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        postprocessingParamsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Scene::Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        postprocessingDescriptorLayout = vireo->createDescriptorLayout();
        postprocessingDescriptorLayout->add(POSTPROCESSING_BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        postprocessingDescriptorLayout->add(POSTPROCESSING_BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        postprocessingDescriptorLayout->build();

        postprocessingPipelineConfig.resources = vireo->createPipelineResources({ postprocessingDescriptorLayout, skybox.getSamplerDescriptorSLayout() });
        postprocessingPipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        postprocessingPipelineConfig.fragmentShader = vireo->createShaderModule("shaders/voronoi.frag");
        postprocessingPipeline = vireo->createGraphicPipeline(postprocessingPipelineConfig);

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

            frame.postProcessingDescriptorSet = vireo->createDescriptorSet(postprocessingDescriptorLayout);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_PARAMS, postprocessingParamsBuffer);
        }

        depthPrepass.onInit(vireo, swapChain->getFramesInFlight());


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
            colorRenderTarget = frame.postProcessingColorBuffer;
            postprocessingRenderingConfig.colorRenderTargets[0].renderTarget = frame.postProcessingColorBuffer;
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.postProcessingColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->beginRendering(postprocessingRenderingConfig);
            cmdList->bindPipeline(postprocessingPipeline);
            cmdList->bindDescriptors(postprocessingPipeline, {frame.postProcessingDescriptorSet, skybox.getSamplerDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                frame.postProcessingColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::UNDEFINED);
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
        postprocessingParams.imageSize.x = extent.width;
        postprocessingParams.imageSize.y = extent.height;
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearValue);
            frame.postProcessingColorBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::R8G8B8A8_SRGB,
                extent.width,
                extent.height);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_INPUT, frame.colorBuffer->getImage());
        }
        depthPrepass.onResize(swapChain->getExtent());
        skybox.onResize(swapChain->getExtent());
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


    float DeferredApp::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        return static_cast<float>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

}