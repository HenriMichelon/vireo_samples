/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "Libraries.h"
module samples.compute;

namespace samples {

    void ComputeApp::onInit() {
        graphicSubmitQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            graphicSubmitQueue,
            windowHandle,
            vireo::PresentMode::IMMEDIATE);
        paramsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Params));
        paramsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_IMAGE, vireo::DescriptorType::READWRITE_IMAGE);
        descriptorLayout->build();

        pipeline = vireo->createComputePipeline(
            vireo->createPipelineResources( { descriptorLayout }),
            vireo->createShaderModule("shaders/compute.comp")
        );

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.inFlightFence = vireo->createFence(true);
        }
    }

    void ComputeApp::onUpdate() {
        params.time = getCurrentTimeMilliseconds() / 1000.0f;
        paramsBuffer->write(&params);
    }

    void ComputeApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        frame.commandAllocator->reset();
        frame.commandList->begin();
        frame.commandList->setDescriptors({frame.descriptorSet});
        frame.commandList->bindPipeline(pipeline);
        frame.commandList->bindDescriptors(pipeline, {frame.descriptorSet});

        frame.commandList->barrier(
            frame.image,
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::DISPATCH_TARGET);
        frame.commandList->dispatch((frame.image->getWidth() + 7)/8, (frame.image->getHeight() + 7)/8, 1);
        frame.commandList->barrier(
            frame.image,
            vireo::ResourceState::DISPATCH_TARGET,
            vireo::ResourceState::COPY_SRC);

        frame.commandList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        frame.commandList->copy(frame.image, swapChain);
        frame.commandList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);

        frame.commandList->end();

        graphicSubmitQueue->submit(frame.inFlightFence, swapChain, {frame.commandList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void ComputeApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        params.imageSize.x = extent.width;
        params.imageSize.y = extent.height;

        std::vector<std::shared_ptr<const vireo::CommandList>> commandLists;
        for (auto& frame : framesData) {
            frame.image = vireo->createReadWriteImage(
                vireo::ImageFormat::R8G8B8A8_UNORM,
                params.imageSize.x, params.imageSize.y);
            frame.descriptorSet->update(BINDING_IMAGE, frame.image);
            frame.commandList->begin();
            frame.commandList->barrier(frame.image, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_SRC);
            frame.commandList->end();
            commandLists.push_back(frame.commandList);
        }
        graphicSubmitQueue->submit(commandLists);
        graphicSubmitQueue->waitIdle();
    }

    void ComputeApp::onDestroy() {
        graphicSubmitQueue->waitIdle();

        // Save to a file
        const auto& frame = framesData[0];
        frame.commandAllocator->reset();
        frame.commandList->begin();
        const auto buffer = vireo->createBuffer(vireo::BufferType::IMAGE_DOWNLOAD, frame.image->getAlignedImageSize());
        frame.commandList->copy(frame.image, buffer);
        frame.commandList->end();
        graphicSubmitQueue->submit({frame.commandList});
        graphicSubmitQueue->waitIdle();
        buffer->map();
        const auto rowPitch = frame.image->getRowPitch();
        const auto alignedRowPitch = frame.image->getAlignedRowPitch();
        std::vector<uint8_t> imageData(frame.image->getImageSize());
        const auto* source = static_cast<uint8_t*>(buffer->getMappedAddress());
        for (int y = 0; y < frame.image->getHeight(); ++y) {
            memcpy(&imageData[y * rowPitch], &source[y * alignedRowPitch], rowPitch);
        }
        buffer->unmap();
        stbi_write_png("output.png",
            frame.image->getWidth(),
            frame.image->getHeight(),
            frame.image->getPixelSize(frame.image->getFormat()),
            imageData.data(),
            rowPitch);

        swapChain->waitIdle();
        paramsBuffer->unmap();
    }

    float ComputeApp::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        static auto startTime = steady_clock::now();
        const duration<float, std::milli> elapsed = steady_clock::now() - startTime;
        return elapsed.count();
    }

}