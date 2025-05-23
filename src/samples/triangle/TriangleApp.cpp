/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.hellotriangle;

namespace samples {

    void TriangleApp::onInit() {
        const auto& adapterDesc = vireo->getPhysicalDevice()->getDescription();
        std::wcout << adapterDesc.name << L" " << std::to_wstring(adapterDesc.dedicatedVideoMemory / 1024 / 1024) << L"Mb" << std::endl;

        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(pipelineConfig.colorRenderFormats.front(), graphicQueue, windowHandle, vireo::PresentMode::IMMEDIATE);
        renderingConfig.colorRenderTargets[0].swapChain = swapChain;
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

        pipelineConfig.resources = vireo->createPipelineResources();
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/triangle_color.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/triangle_color.frag");
        defaultPipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (uint32_t i = 0; i < framesData.size(); i++) {
            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
            framesData[i].inFlightFence =vireo->createFence(true);
        }

        transferQueue->waitIdle();
    }

    void TriangleApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();
        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(vireo::Viewport{
            .width  = static_cast<float>(swapChain->getExtent().width),
            .height = static_cast<float>(swapChain->getExtent().height)});
        cmdList->setScissors(vireo::Rect{
            .width  = swapChain->getExtent().width,
            .height = swapChain->getExtent().height});
        cmdList->bindPipeline(defaultPipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->draw(triangleVertices.size());
        cmdList->endRendering();

        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void TriangleApp::onResize() {
        swapChain->recreate();
    }

    void TriangleApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
    }

}