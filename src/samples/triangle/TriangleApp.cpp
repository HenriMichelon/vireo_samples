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

        const auto triangleVertices = vector<Vertex> {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        vertexBuffer = renderingBackEnd->createBuffer(
            vireo::Buffer::Type::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandList::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandAllocator->reset();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        pipelineResources["default"] = renderingBackEnd->createPipelineResources({ }, L"default");

        const auto defaultVertexInputLayout = renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes);
        const auto vertexShader = renderingBackEnd->createShaderModule("shaders/triangle_color.vert");
        const auto fragmentShader = renderingBackEnd->createShaderModule("shaders/triangle_color.frag");
        pipelines["default"] = renderingBackEnd->createPipeline(
            pipelineResources["default"],
            defaultVertexInputLayout,
            vertexShader,
            fragmentShader,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i] = renderingBackEnd->createFrameData(i);
            graphicCommandAllocator[i] = renderingBackEnd->createCommandAllocator(vireo::CommandList::GRAPHIC);
            graphicCommandList[i] = graphicCommandAllocator[i]->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto frame = swapChain->getCurrentFrameIndex();
        const auto frameData = framesData[frame];

        if (!swapChain->begin(frameData)) { return; }
        graphicCommandAllocator[frame]->reset();
        const auto commandList = graphicCommandList[frame];
        commandList->begin();
        renderingBackEnd->beginRendering(frameData, commandList);

        commandList->bindPipeline(pipelines["default"]);
        commandList->bindVertexBuffer(vertexBuffer);
        commandList->drawInstanced(3);

        renderingBackEnd->endRendering(commandList);
        swapChain->end(frameData, commandList);
        commandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frameData, {commandList});

        swapChain->present(frameData);
        swapChain->nextSwapChain();
    }

    void TriangleApp::onDestroy() {
        renderingBackEnd->waitIdle();
        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            renderingBackEnd->destroyFrameData(framesData[i]);
        }
    }

}