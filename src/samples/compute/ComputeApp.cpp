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

        image = renderingBackEnd->createReadWriteImage(
            vireo::ImageFormat::R8G8B8A8_UNORM,
            params.imageSize.x, params.imageSize.y);

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_IMAGE, vireo::DescriptorType::READWRITE_IMAGE);
        descriptorLayout->build();

        pipeline = renderingBackEnd->createComputePipeline(
            renderingBackEnd->createPipelineResources( { descriptorLayout }),
            renderingBackEnd->createShaderModule("shaders/gradient.comp")
        );

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].graphicCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].graphicCommandList = framesData[i].graphicCommandAllocator->createCommandList();
        }

        descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout);
        descriptorSet->update(BINDING_PARAMS, paramBuffer);
        descriptorSet->update(BINDING_IMAGE, image, true);

        computeCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::COMPUTE);
        computeCommandList = computeCommandAllocator->createCommandList();

        renderingBackEnd->waitIdle();
    }

    void ComputeApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.frameData)) { return; }
        frame.graphicCommandAllocator->reset();
        frame.graphicCommandList->begin();
        frame.graphicCommandList->barrier(frame.frameData, swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        frame.graphicCommandList->barrier(frame.frameData, swapChain, vireo::ResourceState::COPY_DST, vireo::ResourceState::PRESENT);
        frame.graphicCommandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {frame.graphicCommandList});

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