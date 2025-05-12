/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.hellotriangle;

namespace samples {

    void TextureBufferApp::onInit() {
        graphicSubmitQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats[0],
            graphicSubmitQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);
        renderingConfig.colorRenderTargets[0].swapChain = swapChain;

        const auto ratio = swapChain->getAspectRatio();
        for (auto& vertex : triangleVertices) {
            vertex.pos.y *= ratio;
        }

        vertexBuffer = vireo->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size());
        textures.push_back(vireo->createImage(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            512, 512));
        samplers.push_back(vireo->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            vireo::MipMapMode::NEAREST));

        const auto stagingBuffer = vireo->createBuffer(
            vireo::BufferType::IMAGE_TRANSFER,
            textures[0]->getImageSize()
        );
        generateTextureData(stagingBuffer, textures[0]->getWidth(), textures[0]->getHeight());

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->barrier(textures[0], vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        uploadCommandList->copy(stagingBuffer, textures[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_UBO, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::SAMPLED_IMAGE, textures.size());
        descriptorLayout->build();

        samplersDescriptorLayout = vireo->createSamplerDescriptorLayout();
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER, samplers.size());
        samplersDescriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            pushConstantsDesc);
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/triangle_texture_buffer1.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/triangle_texture_buffer1.frag");
        pipelines["shader1"] = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/triangle_texture_buffer2.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/triangle_texture_buffer2.frag");
        pipelines["shader2"] = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frameData : framesData) {
            frameData.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frameData.samplersDescriptorSet = vireo->createDescriptorSet(samplersDescriptorLayout);

            frameData.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM, sizeof(GlobalUBO));
            frameData.globalUniform->map();

            frameData.descriptorSet->update(BINDING_UBO, frameData.globalUniform);
            frameData.descriptorSet->update(BINDING_TEXTURE, textures);
            frameData.samplersDescriptorSet->update(BINDING_SAMPLERS, samplers);

            frameData.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frameData.commandList = frameData.commandAllocator->createCommandList();

            frameData.inFlightFence =vireo->createFence(true);
        }

        transferQueue->waitIdle();
        framesData[0].commandList->begin();
        framesData[0].commandList->barrier(textures[0], vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);
        framesData[0].commandList->end();
        graphicSubmitQueue->submit({framesData[0].commandList});
        graphicSubmitQueue->waitIdle();
    }

    void TextureBufferApp::onUpdate() {
        constexpr float translationSpeed = 0.005f;
        constexpr float offsetBounds = 1.25f;
        globalUbo.offset.x += translationSpeed;
        if (globalUbo.offset.x > offsetBounds) {
            globalUbo.offset.x = -offsetBounds;
        }
        pushConstants.color += 0.005f * colorIncrement;
        if ((pushConstants.color.x > 0.5f) || (pushConstants.color.x < 0.0f)) {
            colorIncrement = -colorIncrement;
        }
    }

    void TextureBufferApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        frame.globalUniform->write(&globalUbo);

        frame.commandAllocator->reset();
        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(swapChain->getExtent());
        cmdList->setScissors(swapChain->getExtent());
        cmdList->setDescriptors({frame.descriptorSet, frame.samplersDescriptorSet});

        cmdList->bindPipeline(pipelines["shader1"]);
        cmdList->bindDescriptors(pipelines["shader1"], {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->draw(triangleVertices.size());

        cmdList->bindPipeline(pipelines["shader2"]);
        cmdList->bindDescriptors(pipelines["shader2"], {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->pushConstants(pipelineConfig.resources, pushConstantsDesc, &pushConstants);
        cmdList->draw(triangleVertices.size(), 2);

        cmdList->endRendering();
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicSubmitQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void TextureBufferApp::onDestroy() {
        graphicSubmitQueue->waitIdle();
        swapChain->waitIdle();
        for (const auto& frame : framesData) {
            frame.globalUniform->unmap();
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    void TextureBufferApp::generateTextureData(
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
                pData[n + 3] = 0x00;    // A
            }
        }
        destination->unmap();
    }

}