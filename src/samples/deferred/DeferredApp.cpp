/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
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
        const auto keyCode = static_cast<KeyCodes>(key);
        if (keyCode == KeyCodes::P) {
            postProcessing.toggleDisplayEffect();
            return;
        }
        scene.onKeyDown(key);
    }

    void DeferredApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"MainQueue");
        swapChain = vireo->createSwapChain(
            RENDER_FORMAT,
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        depthPrepass.onInit(vireo, true, swapChain->getFramesInFlight());
        lightingPass.onInit(vireo, RENDER_FORMAT, scene, depthPrepass, swapChain->getFramesInFlight());

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());
        skybox.onInit(vireo, uploadCommandList, RENDER_FORMAT, depthPrepass, swapChain->getFramesInFlight());
        uploadCommandList->end();
        graphicQueue->submit({uploadCommandList});

        gbufferPass.onInit(vireo, scene, depthPrepass, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, RENDER_FORMAT, swapChain->getFramesInFlight());
        postProcessing.toggleGammaCorrection();

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true);
        }

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

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();
        gbufferPass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            depthPrepass,
            cmdList);
        lightingPass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            depthPrepass,
            gbufferPass,
            cmdList,
            frame.colorBuffer);
        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            false,
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
            depthPrepass.isWithStencil() ?
                    vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL :
                    vireo::ResourceState::RENDER_TARGET_DEPTH,
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

    void DeferredApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        const auto cmdAlloc = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        auto cmdList = cmdAlloc->createCommandList();
        cmdList->begin();
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                skybox.getClearValue());
        }
        depthPrepass.onResize(extent);
        postProcessing.onResize(extent);
        gbufferPass.onResize(extent, cmdList);
        cmdList->end();
        graphicQueue->submit({cmdList});
        graphicQueue->waitIdle();
    }

    void DeferredApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        gbufferPass.onDestroy();
        depthPrepass.onDestroy();
        skybox.onDestroy();
    }

}