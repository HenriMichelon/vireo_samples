/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.compute;

APP(make_shared<samples::ComputeApp>(), L"Hello Compute", 0, 0, false);

namespace samples {

    void ComputeApp::onInit() {
        paramsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Params), 1, 256);
        paramsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_IMAGE, vireo::DescriptorType::READWRITE_IMAGE);
        descriptorLayout->build();

        pipeline = vireo->createComputePipeline(
            vireo->createPipelineResources( { descriptorLayout }),
            vireo->createShaderModule("shaders/compute.comp")
        );

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
            framesData[i].descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            framesData[i].descriptorSet->update(BINDING_PARAMS, paramsBuffer);
            framesData[i].inFlightFence =vireo->createFence();
        }
    }

    void ComputeApp::onUpdate() {
        params.time = getCurrentTimeMilliseconds() / 1000.0f;
        paramsBuffer->write(&params);
    }

    void ComputeApp::onRender() {
        const auto swapChain = vireo->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        frame.commandAllocator->reset();
        frame.commandList->begin();
        frame.commandList->bindPipeline(pipeline);
        frame.commandList->bindDescriptors(pipeline, {frame.descriptorSet});

        frame.commandList->barrier(frame.image, vireo::ResourceState::COPY_SRC, vireo::ResourceState::DISPATCH_TARGET);
        frame.commandList->dispatch((frame.image->getWidth() + 7)/8, (frame.image->getHeight() + 7)/8, 1);
        frame.commandList->barrier(frame.image, vireo::ResourceState::DISPATCH_TARGET, vireo::ResourceState::COPY_SRC);

        frame.commandList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        frame.commandList->copy(frame.image, swapChain);
        frame.commandList->barrier(swapChain, vireo::ResourceState::COPY_DST, vireo::ResourceState::PRESENT);

        frame.commandList->end();

        vireo->getGraphicCommandQueue()->submit(frame.inFlightFence, swapChain, {frame.commandList});

        swapChain->present();
        swapChain->nextSwapChain();
    }

    void ComputeApp::onResize() {
        const auto swapChain = vireo->getSwapChain();
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        params.imageSize.x = extent.width;
        params.imageSize.y = extent.height;

        vector<shared_ptr<const vireo::CommandList>> commandLists;
        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].image = vireo->createReadWriteImage(
                vireo::ImageFormat::R8G8B8A8_UNORM,
                params.imageSize.x, params.imageSize.y);
            framesData[i].descriptorSet->update(BINDING_IMAGE, framesData[i].image, true);
            framesData[i].commandList->begin();
            framesData[i].commandList->barrier(framesData[i].image, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_SRC);
            framesData[i].commandList->end();
            commandLists.push_back(framesData[i].commandList);
        }
        vireo->getGraphicCommandQueue()->submit(commandLists);
        vireo->getGraphicCommandQueue()->waitIdle();
    }

    void ComputeApp::onDestroy() {
        vireo->waitIdle();
        paramsBuffer->unmap();
    }

    float ComputeApp::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        static auto startTime = steady_clock::now();
        const duration<float, std::milli> elapsed = steady_clock::now() - startTime;
        return elapsed.count();
    }

}