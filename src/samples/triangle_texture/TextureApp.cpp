/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.hellotexture;

namespace samples {

    void TextureApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"Graphic");
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats[0],
            graphicQueue,
            windowHandle,
            vireo::PresentMode::IMMEDIATE);
        renderingConfig.colorRenderTargets[0].swapChain = swapChain;

        const auto ratio = swapChain->getAspectRatio();
        for (auto& vertex : triangleVertices) {
            vertex.pos.y *= ratio;
        }

        vertexBuffer = vireo->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            L"TriangleVertexBuffer");
        texture = vireo->createImage(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            512, 512, 1, 1,
            L"CheckerBoardTexture");
        sampler = vireo->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            vireo::MipMapMode::NEAREST);

        const auto stagingBuffer = vireo->createBuffer(
            vireo::BufferType::IMAGE_TRANSFER,
            texture->getImageSize()
        );
        generateTextureData(stagingBuffer, texture->getWidth(), texture->getHeight());

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->barrier(texture, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        uploadCommandList->copy(stagingBuffer, texture);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        descriptorLayout = vireo->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        samplersDescriptorLayout = vireo->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER);
        samplersDescriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            {},
            L"Default");
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/triangle_texture.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/triangle_texture.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig, L"default");

        framesData.resize(swapChain->getFramesInFlight());
        for (uint32_t i = 0; i < framesData.size(); i++) {
            framesData[i].descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"Global " + std::to_wstring(i));
            framesData[i].samplersDescriptorSet = vireo->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + std::to_wstring(i));

            framesData[i].descriptorSet->update(BINDING_TEXTURE, texture);
            framesData[i].samplersDescriptorSet->update(BINDING_SAMPLERS, sampler);

            framesData[i].commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();

            framesData[i].inFlightFence =vireo->createFence(true);
        }

        transferQueue->waitIdle();
        framesData[0].commandList->begin();
        framesData[0].commandList->barrier(texture, vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);
        framesData[0].commandList->end();
        graphicQueue->submit({framesData[0].commandList});
        graphicQueue->waitIdle();
    }

    void TextureApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(swapChain->getExtent());
        cmdList->setScissors(swapChain->getExtent());
        cmdList->setDescriptors({frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->draw(triangleVertices.size());
        cmdList->endRendering();

        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void TextureApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    void TextureApp::generateTextureData(
        const std::shared_ptr<vireo::Buffer>&destination,
        const uint32_t width,
        const uint32_t height) {
        const auto rowPitch = width * 4;
        const auto cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
        const auto cellHeight = width >> 3;    // The height of a cell in the checkerboard texture.
        const auto textureSize = rowPitch * height;
        destination->map();
        const auto pData = static_cast<unsigned char*>(destination->getMappedAddress());

        for (int n = 0; n < textureSize; n += 4) {
            const auto x = n % rowPitch;
            const auto y = n / rowPitch;
            const auto i = x / cellPitch;
            const auto j = y / cellHeight;

            if (i % 2 == j % 2) {
                pData[n] = 0x00;        // R
                pData[n + 1] = 0x00;    // G
                pData[n + 2] = 0x00;    // B
                pData[n + 3] = 0xff;    // A
            }
            else {
                pData[n] = 0xff;        // R
                pData[n + 1] = 0xff;    // G
                pData[n + 2] = 0xff;    // B
                pData[n + 3] = 0xff;    // A
            }
        }
        destination->unmap();
    }

}