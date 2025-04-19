/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellomsaa;

APP(make_shared<samples::TriangleApp>(), L"Hello MSAA", 1280, 720);

namespace samples {

    void MsaaApp::onInit() {
        graphicSubmitQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(defaultPipelineConfig.colorRenderFormat, graphicSubmitQueue, vireo::PresentMode::IMMEDIATE);
        const auto ratio = swapChain->getAspectRatio();
        for (auto& vertex : triangleVertices) {
            vertex.pos.y *= ratio;
        }

        vertexBuffer = vireo->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size());

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        defaultPipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ }, {}),
            vireo->createVertexLayout(sizeof(Vertex), vertexAttributes),
            vireo->createShaderModule("shaders/triangle_color.vert"),
            vireo->createShaderModule("shaders/triangle_color.frag"),
            defaultPipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (uint32_t i = 0; i < framesData.size(); i++) {
            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
            framesData[i].inFlightFence =vireo->createFence();
            framesData[i].msaaRenderTarget = vireo->createRenderTarget(swapChain, defaultPipelineConfig.msaa);
        }

        transferQueue->waitIdle();
        uploadCommandList->cleanup();
    }

    void MsaaApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET);
        cmdList->barrier(frame.msaaRenderTarget, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET);
        cmdList->beginRendering(frame.msaaRenderTarget, swapChain, clearColor);
        // cmdList->beginRendering(swapChain, clearColor);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});

        cmdList->bindPipeline(defaultPipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->drawInstanced(triangleVertices.size());

        cmdList->endRendering();
        cmdList->barrier(frame.msaaRenderTarget, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicSubmitQueue->submit(frame.inFlightFence, swapChain, {cmdList});

        swapChain->present();
        swapChain->nextSwapChain();
    }

    void MsaaApp::onResize() {
        swapChain->recreate();
    }

    void MsaaApp::onDestroy() {
        graphicSubmitQueue->waitIdle();
        vireo->waitIdle();
    }

}