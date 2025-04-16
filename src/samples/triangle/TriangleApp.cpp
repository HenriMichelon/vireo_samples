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
        vertexBuffer = renderingBackEnd->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        defaultPipeline = renderingBackEnd->createGraphicPipeline(
            renderingBackEnd->createPipelineResources({ }, {}, L"default"),
            renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes),
            renderingBackEnd->createShaderModule("shaders/triangle_color.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_color.frag"),
            defaultPipelineConfig,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
        }

        // renderTarget = renderingBackEnd->createRenderTarget(
        //     vireo::ImageFormat::R8G8B8A8_SRGB,
        //     1024,
        //     1024);

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.frameData)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
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

        cmdList->endRendering(frame.frameData, swapChain);
        // cmdList->endRendering(renderTarget);
        cmdList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {cmdList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void TriangleApp::onDestroy() {
        renderingBackEnd->waitIdle();
        for (auto& data : framesData) {
            renderingBackEnd->destroyFrameData(data.frameData);
        }
    }

}