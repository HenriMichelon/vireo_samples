/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotriangle;

APP(make_shared<samples::TriangleApp>(), L"Hello Triangle", 800, 600);

namespace samples {

    void TriangleApp::onInit() {
        vertexBuffer = vireo->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->end();
        vireo->getTransferCommandQueue()->submit({uploadCommandList});

        defaultPipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ }, {}, L"default"),
            vireo->createVertexLayout(sizeof(Vertex), vertexAttributes),
            vireo->createShaderModule("shaders/triangle_color.vert"),
            vireo->createShaderModule("shaders/triangle_color.frag"),
            defaultPipelineConfig,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].frameData = vireo->createFrameData(i);
            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
        }

        // renderTarget = vireo->createRenderTarget(
        //     vireo::ImageFormat::R8G8B8A8_SRGB,
        //     1024,
        //     1024);

        vireo->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onRender() {
        const auto swapChain = vireo->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.frameData)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(frame.frameData, swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET);
        cmdList->beginRendering(frame.frameData, swapChain, clearColor);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});
        // cmdList->beginRendering(renderTarget, clearColor);
        // const auto extent = vireo::Extent{renderTarget->getImage()->getWidth(), renderTarget->getImage()->getHeight()};
        // cmdList->setViewports(1, {extent});
        // cmdList->setScissors(1, {extent});
        cmdList->setPrimitiveTopology(vireo::PrimitiveTopology::TRIANGLE_LIST);

        cmdList->bindPipeline(defaultPipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->drawInstanced(triangleVertices.size());

        cmdList->endRendering();
        cmdList->barrier(frame.frameData, swapChain, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::PRESENT);
        cmdList->end();

        vireo->getGraphicCommandQueue()->submit(frame.frameData, {cmdList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void TriangleApp::onDestroy() {
        vireo->waitIdle();
        for (auto& data : framesData) {
            vireo->destroyFrameData(data.frameData);
        }
    }

}