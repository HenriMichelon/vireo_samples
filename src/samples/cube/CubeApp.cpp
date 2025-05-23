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
        const auto keyCode = static_cast<KeyScanCodes>(key);
        graphicQueue->waitIdle();
        postProcessing.onKeyDown(keyCode);
        scene.onKeyDown(keyCode);
    }

    void CubeApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            RENDER_FORMAT,
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        samplers.onInit(vireo);

        auto stagingBuffers = std::vector<std::shared_ptr<vireo::Buffer>>();
        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        scene.onInit(vireo, uploadCommandList, stagingBuffers, swapChain->getAspectRatio());
        depthPrepass.onInit(vireo, scene, false, swapChain->getFramesInFlight());
        skybox.onInit(vireo, uploadCommandList, RENDER_FORMAT, depthPrepass, samplers, swapChain->getFramesInFlight());
        uploadCommandList->end();
        graphicQueue->submit({uploadCommandList});

        colorPass.onInit(vireo, RENDER_FORMAT, scene, depthPrepass, samplers, swapChain->getFramesInFlight());
        postProcessing.onInit(vireo, RENDER_FORMAT, samplers, swapChain->getFramesInFlight());

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true);
            frame.semaphore = vireo->createSemaphore(vireo::SemaphoreType::TIMELINE, L"Main timeline");
        }
        graphicQueue->waitIdle();
        stagingBuffers.clear();
    }

    void CubeApp::onRender() {
        const auto frameIndex = swapChain->getCurrentFrameIndex();
        const auto& frame = framesData[frameIndex];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        depthPrepass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            frame.semaphore,
            graphicQueue);

        skybox.onRender(
            frameIndex,
            swapChain->getExtent(),
            true,
            depthPrepass,
            samplers,
            frame.colorBuffer,
            frame.semaphore,
            graphicQueue);

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();

        colorPass.onRender(
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
                vireo::ResourceState::RENDER_TARGET_DEPTH,
                vireo::ResourceState::UNDEFINED);

        auto colorBuffer = postProcessing.getColorBuffer(frameIndex);
        if (colorBuffer == nullptr) {
            colorBuffer = frame.colorBuffer;
        }
        cmdList->barrier(
           colorBuffer,
           vireo::ResourceState::RENDER_TARGET_COLOR,
           vireo::ResourceState::COPY_SRC);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        cmdList->copy(colorBuffer, swapChain);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);
        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::UNDEFINED);
        cmdList->end();

        frame.semaphore->decrementValue();
        graphicQueue->submit(
            frame.semaphore,
            {vireo::WaitStage::VERTEX_SHADER, vireo::WaitStage::FRAGMENT_SHADER},
            frame.inFlightFence,
            swapChain,
            {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
        frame.semaphore->incrementValue();
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
    }

}