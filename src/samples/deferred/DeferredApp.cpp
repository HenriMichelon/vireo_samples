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
        const auto keyCode = static_cast<KeyScanCodes>(key);
        graphicQueue->waitIdle();
        postProcessing.onKeyDown(keyCode);
        scene.onKeyDown(keyCode);
    }

    void DeferredApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"MainQueue");
        swapChain = vireo->createSwapChain(
            RENDER_FORMAT,
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        samplers.onInit(vireo);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());
        depthPrepass.onInit(vireo, scene, true, swapChain->getFramesInFlight());
        lightingPass.onInit(vireo, RENDER_FORMAT, scene, depthPrepass, samplers, swapChain->getFramesInFlight());
        transparencyPass.onInit(vireo, RENDER_FORMAT, scene, depthPrepass, samplers, swapChain->getFramesInFlight());
        skybox.onInit(vireo, uploadCommandList, RENDER_FORMAT, depthPrepass, samplers, swapChain->getFramesInFlight());
        uploadCommandList->end();
        graphicQueue->submit({uploadCommandList});

        gbufferPass.onInit(vireo, scene, depthPrepass, samplers, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, RENDER_FORMAT, samplers, swapChain->getFramesInFlight());

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
            samplers,
            cmdList);
        cmdList->barrier(
           frame.colorBuffer,
           vireo::ResourceState::UNDEFINED,
           vireo::ResourceState::RENDER_TARGET_COLOR);
        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            false,
            depthPrepass,
            samplers,
            frame.colorBuffer,
            cmdList);
        lightingPass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            depthPrepass,
            gbufferPass,
            samplers,
            cmdList,
            frame.colorBuffer);
        transparencyPass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            depthPrepass,
            samplers,
            cmdList,
            frame.colorBuffer);
        postProcessing.onRender(
            frameIndex,
            swapChain->getExtent(),
            samplers,
            cmdList,
            frame.colorBuffer);

        cmdList->barrier(
            depthPrepass.getDepthBuffer(frameIndex),
            depthPrepass.isWithStencil() ?
                    vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL :
                    vireo::ResourceState::RENDER_TARGET_DEPTH,
            vireo::ResourceState::UNDEFINED);

        auto colorBuffer = postProcessing.getColorBuffer(frameIndex);
        if (colorBuffer == nullptr) {
            colorBuffer = frame.colorBuffer;
        }
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::COPY_SRC);
        cmdList->copy(colorBuffer, swapChain);
        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::UNDEFINED);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);
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
        transparencyPass.onResize(extent, cmdList);
        cmdList->end();
        graphicQueue->submit({cmdList});
        graphicQueue->waitIdle();
    }

    void DeferredApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
    }

}