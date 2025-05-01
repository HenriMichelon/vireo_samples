/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include <glm/gtc/matrix_transform.hpp>
#include "Libraries.h"
module samples.deferred;

import samples.common.global;

namespace samples {

    void DeferredApp::onUpdate() {
        scene.onUpdate();
        skybox.onUpdate(scene);
        postProcessing.onUpdate();
    }

    void DeferredApp::onKeyDown(const uint32_t key) {
        scene.onKeyDown(key);
    }

    void DeferredApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"MainQueue");
        swapChain = vireo->createSwapChain(
            RENDER_FORMAT,
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());
        skybox.onInit(vireo, uploadCommandList, graphicQueue, swapChain->getFramesInFlight());
        uploadCommandList->end();
        graphicQueue->submit({uploadCommandList});

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true);
        }
        colorPass.onInit(vireo, scene, swapChain->getFramesInFlight());
        depthPrepass.onInit(vireo, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, swapChain->getFramesInFlight());

        graphicQueue->waitIdle();
    }

    void DeferredApp::onRender() {
        const auto frameIndex = swapChain->getCurrentFrameIndex();
        const auto& frame = framesData[frameIndex];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        depthPrepass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            graphicQueue);

        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            depthPrepass,
            frame.colorBuffer,
            graphicQueue);

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();
        colorPass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            depthPrepass,
            cmdList,
            frame.colorBuffer);
        postProcessing.onRender(
            frameIndex,
            swapChain->getExtent(),
            cmdList,
            frame.colorBuffer);

        cmdList->barrier(
            depthPrepass.getDepthBuffer(frameIndex),
            vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL_READ,
            vireo::ResourceState::UNDEFINED);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        cmdList->copy(postProcessing.getColorBuffer(frameIndex), swapChain);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);
        cmdList->barrier(
            postProcessing.getColorBuffer(frameIndex),
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::UNDEFINED);
        cmdList->end();

        graphicQueue->submit(
            skybox.getSemaphore(frameIndex),
            vireo::WaitStage::TRANSFER,
            frame.inFlightFence,
            swapChain,
            {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void DeferredApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                skybox.getClearValue());
        }
        depthPrepass.onResize(extent);
        skybox.onResize(extent);
        postProcessing.onResize(extent);
    }

    void DeferredApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        colorPass.onDestroy();
        depthPrepass.onDestroy();
        skybox.onDestroy();
    }


}