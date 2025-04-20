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

APP(make_shared<samples::CubeApp>(), L"Hello Cube", 1280, 720);

namespace samples {

    void CubeApp::onInit() {
        graphicSubmitQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormat,
            graphicSubmitQueue,
            vireo::PresentMode::VSYNC);

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        global.view = lookAt(cameraPos, cameraTarget, up);
        global.projection = perspective(radians(75.0f), swapChain->getAspectRatio(), 0.1f, 100.0f);
        globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        globalBuffer->map();
        globalBuffer->write(&global, sizeof(Global));
        globalBuffer->unmap();

        model.transform= mat4(1.0f);
        modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model));
        modelBuffer->map();
        modelBuffer->write(&model, sizeof(Model));

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
        for (uint32_t i = 0; i < framesData.size(); i++) {
            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
            framesData[i].inFlightFence =vireo->createFence();
            framesData[i].descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            framesData[i].descriptorSet->update(BINDING_GLOBAL, globalBuffer);
            framesData[i].descriptorSet->update(BINDING_MODEL, modelBuffer);
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
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET);
        cmdList->beginRendering(swapChain, clearColor);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});

        cmdList->bindPipeline(pipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet});
        cmdList->drawInstanced(cubeVertices.size());

        cmdList->endRendering();
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicSubmitQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextSwapChain();
    }

    void CubeApp::onResize() {
        swapChain->recreate();
    }

    void CubeApp::onDestroy() {
        graphicSubmitQueue->waitIdle();
        vireo->waitIdle();
        modelBuffer->unmap();
    }

}