/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include <glm/gtc/matrix_transform.hpp>
#include "Macros.h"
module samples.hellocube;

APP(make_shared<samples::CubeApp>(), L"Hello Cube", 0, 0);

namespace samples {

    void CubeApp::onUpdate() {
        constexpr  float angle = radians(-1.0);
        model.transform = glm::rotate(model.transform, angle, AXIS_X);
        model.transform = glm::rotate(model.transform, angle, AXIS_Y);
        modelBuffer->write(&model, sizeof(Model));
    }

    void CubeApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormat,
            graphicQueue,
            vireo::PresentMode::VSYNC);
        renderingConfig.swapChain = swapChain;

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        global.view = lookAt(cameraPos, cameraTarget, up);
        global.projection = perspective(radians(75.0f), swapChain->getAspectRatio(), 0.1f, 100.0f);
        globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        globalBuffer->map();
        globalBuffer->write(&global, sizeof(Global));
        globalBuffer->unmap();

        modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model));
        modelBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->build();

        pipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ descriptorLayout }),
            vireo->createVertexLayout(sizeof(Vertex), vertexAttributes),
            vireo->createShaderModule("shaders/cube_color_mvp.vert"),
            vireo->createShaderModule("shaders/cube_color_mvp.frag"),
            pipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, modelBuffer);
            frame.msaaBuffer = vireo->createRenderTarget(swapChain, renderingConfig.clearColorValue, pipelineConfig.msaa);
            frame.depthBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::D32_SFLOAT,
                swapChain->getExtent().width,
                swapChain->getExtent().height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
            frame.msaaDepthBuffer = vireo->createRenderTarget(
                frame.depthBuffer->getImage()->getFormat(),
                frame.depthBuffer->getImage()->getWidth(),
                frame.depthBuffer->getImage()->getHeight(),
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue,
                pipelineConfig.msaa);
        }

        transferQueue->waitIdle();
        uploadCommandList->cleanup();
    }

    void CubeApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(frame.msaaBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(frame.depthBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
        cmdList->barrier(frame.msaaDepthBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
        renderingConfig.multisampledColorRenderTarget = frame.msaaBuffer;
        renderingConfig.depthRenderTarget = frame.depthBuffer;
        renderingConfig.multisampledDepthRenderTarget = frame.msaaDepthBuffer;
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});

        cmdList->bindPipeline(pipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindIndexBuffer(indexBuffer);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet});
        cmdList->drawIndexed(cubeIndices.size());

        cmdList->endRendering();
        cmdList->barrier(frame.msaaBuffer, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(frame.depthBuffer, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(frame.msaaDepthBuffer, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextSwapChain();
    }

    void CubeApp::onResize() {
        swapChain->recreate();
        for (auto& frame : framesData) {
            frame.msaaBuffer = vireo->createRenderTarget(swapChain, renderingConfig.clearColorValue, pipelineConfig.msaa);
            frame.depthBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::D32_SFLOAT,
                swapChain->getExtent().width,
                swapChain->getExtent().height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
            frame.msaaDepthBuffer = vireo->createRenderTarget(
                frame.depthBuffer->getImage()->getFormat(),
                frame.depthBuffer->getImage()->getWidth(),
                frame.depthBuffer->getImage()->getHeight(),
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue,
                pipelineConfig.msaa);
        }
    }

    void CubeApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        modelBuffer->unmap();
    }

}