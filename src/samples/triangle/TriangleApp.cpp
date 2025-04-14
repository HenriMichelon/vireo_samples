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
        renderingBackEnd->setClearColor( 0.0f, 0.2f, 0.4f);
        const auto aspectRatio = renderingBackEnd->getSwapChain()->getAspectRatio();

        triangleVertices = {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

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

        pipelines["default"] = renderingBackEnd->createPipeline(
            renderingBackEnd->createPipelineResources({ }, L"default"),
            renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes),
            renderingBackEnd->createShaderModule("shaders/triangle_color.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_color.frag"),
            pipelineConfig,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->begin(frame.frameData)) { return; }
        frame.commandAllocator->reset();
        frame.commandList->begin();
        renderingBackEnd->beginRendering(frame.frameData, frame.commandList);

        frame.commandList->bindPipeline(pipelines["default"]);
        frame.commandList->bindVertexBuffer(vertexBuffer);
        frame.commandList->drawInstanced(triangleVertices.size());

        renderingBackEnd->endRendering(frame.commandList);
        swapChain->end(frame.frameData, frame.commandList);
        frame.commandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {frame.commandList});

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