/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.compute;

APP(make_shared<samples::ComputeApp>(), L"Hello Compute", 800, 600);

namespace samples {

    void ComputeApp::onInit() {
        params.imageSize.x = renderingBackEnd->getSwapChain()->getExtent().width;
        params.imageSize.y = renderingBackEnd->getSwapChain()->getExtent().height;
        paramBuffer = renderingBackEnd->createBuffer(vireo::BufferType::UNIFORM,sizeof(Params), 1, 256);
        paramBuffer->map();
        paramBuffer->write(&params);
        paramBuffer->unmap();

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_IMAGE, vireo::DescriptorType::READWRITE_IMAGE);
        descriptorLayout->build();

        pipeline = renderingBackEnd->createComputePipeline(
            renderingBackEnd->createPipelineResources( { descriptorLayout }),
            renderingBackEnd->createShaderModule("shaders/gradient.comp")
        );

        vector<shared_ptr<const vireo::CommandList>> commandLists;
        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
            framesData[i].image = renderingBackEnd->createReadWriteImage(
                vireo::ImageFormat::R8G8B8A8_UNORM,
                params.imageSize.x, params.imageSize.y);
            framesData[i].descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout);
            framesData[i].descriptorSet->update(BINDING_PARAMS, paramBuffer);
            framesData[i].descriptorSet->update(BINDING_IMAGE, framesData[i].image, true);

            framesData[i].commandList->begin();
            framesData[i].commandList->barrier(framesData[i].image, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_SRC);
            framesData[i].commandList->end();
            commandLists.push_back(framesData[i].commandList);
        }
        renderingBackEnd->getGraphicCommandQueue()->submit(commandLists);
        renderingBackEnd->getGraphicCommandQueue()->waitIdle();
    }

    void ComputeApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.frameData)) { return; }

        frame.commandAllocator->reset();
        frame.commandList->begin();
        frame.commandList->bindPipeline(pipeline);
        frame.commandList->bindDescriptors(pipeline, {frame.descriptorSet});

        frame.commandList->barrier(frame.image, vireo::ResourceState::COPY_SRC, vireo::ResourceState::DISPATCH_TARGET);
        frame.commandList->dispatch((frame.image->getWidth() + 7)/8, (frame.image->getHeight() + 7)/8, 1);
        frame.commandList->barrier(frame.image, vireo::ResourceState::DISPATCH_TARGET, vireo::ResourceState::COPY_SRC);

        frame.commandList->barrier(frame.frameData, swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        frame.commandList->copy(frame.image, frame.frameData, swapChain);
        frame.commandList->barrier(frame.frameData, swapChain, vireo::ResourceState::COPY_DST, vireo::ResourceState::PRESENT);

        frame.commandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {frame.commandList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void ComputeApp::onDestroy() {
        renderingBackEnd->waitIdle();
        for (auto& data : framesData) {
            renderingBackEnd->destroyFrameData(data.frameData);
        }
    }

}