/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.cube;

namespace samples {

    void CubeApp::onUpdate() {
        scene.onUpdate();
        skybox.onUpdate(scene);
        postProcessing.onUpdate();
    }

    void CubeApp::onKeyDown(const uint32_t key) {
        const auto keyCode = static_cast<KeyCodes>(key);
        if (keyCode == KeyCodes::P) {
            postProcessing.toggleDisplayEffect();
            return;
        }
        scene.onKeyDown(key);
    }

    void CubeApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            RENDER_FORMAT,
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());
        depthPrepass.onInit(vireo, scene, false, swapChain->getFramesInFlight());
        skybox.onInit(vireo, uploadCommandList, RENDER_FORMAT, depthPrepass, swapChain->getFramesInFlight());
        uploadCommandList->end();
        graphicQueue->submit({uploadCommandList});

        colorPass.onInit(vireo, RENDER_FORMAT, scene, depthPrepass, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, RENDER_FORMAT, swapChain->getFramesInFlight());

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true);
        }
        graphicQueue->waitIdle();
    }

    void CubeApp::onRender() {
        const auto frameIndex = swapChain->getCurrentFrameIndex();
        const auto& frame = framesData[frameIndex];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        depthPrepass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
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

        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            true,
            depthPrepass,
            frame.colorBuffer,
            cmdList);

        postProcessing.onRender(
            frameIndex,
            swapChain->getExtent(),
            cmdList,
            frame.colorBuffer);

        cmdList->barrier(
            depthPrepass.getDepthBuffer(frameIndex),
                    vireo::ResourceState::RENDER_TARGET_DEPTH_READ,
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
            depthPrepass.getSemaphore(frameIndex),
            vireo::WaitStage::TRANSFER,
            frame.inFlightFence,
            swapChain,
            {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void CubeApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                colorPass.getClearValue());
        }
        depthPrepass.onResize(extent);
        postProcessing.onResize(extent);
    }

    void CubeApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        colorPass.onDestroy();
        depthPrepass.onDestroy();
        skybox.onDestroy();
    }

}