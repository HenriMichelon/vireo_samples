/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotriangle;

APP(make_shared<samples::TriangleApp>(), L"Hello Triangle", 1280, 720);

namespace samples {

    void TriangleApp::onInit() {
        swapChain = vireo->createSwapChain(defaultPipelineConfig.colorRenderFormat, vireo::PresentMode::IMMEDIATE);
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
        vireo->getTransferCommandQueue()->submit({uploadCommandList});

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

        vireo->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onRender() {
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

        vireo->getGraphicCommandQueue()->submit(frame.inFlightFence, swapChain, {cmdList});

        swapChain->present();
        swapChain->nextSwapChain();
    }

    void TriangleApp::onResize() {
        swapChain->recreate();
    }

    void TriangleApp::onDestroy() {
        vireo->waitIdle();
    }

}