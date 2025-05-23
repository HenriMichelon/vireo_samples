/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.hellomsaa;

namespace samples {

    void MsaaApp::onInit() {
        graphicSubmitQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats.front(),
            graphicSubmitQueue,
            windowHandle,
            vireo::PresentMode::IMMEDIATE);
        renderingConfig.colorRenderTargets[0].swapChain = swapChain;

        const auto ratio = swapChain->getAspectRatio();
        for (auto& vertex : triangleVertices) {
            vertex.pos.y *= ratio;
        }

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX, sizeof(Vertex), triangleVertices.size());
        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        pipelineConfig.resources = vireo->createPipelineResources();
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/triangle_color.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/triangle_color.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true);
        }

        transferQueue->waitIdle();
    }

    void MsaaApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];
        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(frame.msaaRenderTarget, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);

        renderingConfig.colorRenderTargets[0].multisampledRenderTarget = frame.msaaRenderTarget;
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(vireo::Viewport{
            .width  = static_cast<float>(swapChain->getExtent().width),
            .height = static_cast<float>(swapChain->getExtent().height)});
        cmdList->setScissors(vireo::Rect{
            .width  = swapChain->getExtent().width,
            .height = swapChain->getExtent().height});
        cmdList->bindPipeline(pipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->draw(triangleVertices.size());
        cmdList->endRendering();

        cmdList->barrier(frame.msaaRenderTarget, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicSubmitQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void MsaaApp::onResize() {
        swapChain->recreate();
        for (auto& frame : framesData) {
            frame.msaaRenderTarget = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearValue,
                pipelineConfig.msaa);
        }
    }

    void MsaaApp::onDestroy() {
        graphicSubmitQueue->waitIdle();
        swapChain->waitIdle();
    }

}