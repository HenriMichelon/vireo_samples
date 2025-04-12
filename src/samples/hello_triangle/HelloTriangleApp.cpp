/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotriangle;

import samples.win32;

APP(std::make_shared<samples::HelloTriangleApp>(), L"Hello Triangle", 800, 600);

namespace samples {

    void HelloTriangleApp::onInit() {
        renderingBackEnd->setClearColor( 0.0f, 0.2f, 0.4f);
        const auto aspectRatio = renderingBackEnd->getSwapChain()->getAspectRatio();

        const auto triangleVertices = std::vector<Vertex> {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        vertexBuffer = renderingBackEnd->createBuffer(
            backend::Buffer::Type::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");
        textures.push_back(renderingBackEnd->createImage(
            backend::ImageFormat::R8G8B8A8_SRGB,
            TextureWidth,
            TextureHeight,
            L"CheckerBoardTexture1"));
        textures.push_back(renderingBackEnd->createImage(
            backend::ImageFormat::R8G8B8A8_SRGB,
            TextureWidth / 2,
            TextureHeight / 2,
            L"CheckerBoardTexture2"));

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(backend::CommandList::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->reset();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        for (auto & texture : textures) {
            uploadCommandList->upload(texture, generateTextureData().data());
        }
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        uboBuffer1 = renderingBackEnd->createBuffer(
            backend::Buffer::UNIFORM,
            sizeof(GlobalUBO1),
            1, 256,
            L"UBO1");
        uboBuffer1->map();
        uboBuffer2 = renderingBackEnd->createBuffer(
            backend::Buffer::UNIFORM,
            sizeof(GlobalUBO2),
            1, 256,
            L"UBO2");
        uboBuffer2->map();

        samplers.push_back(renderingBackEnd->createSampler(
            backend::Filter::NEAREST,
            backend::Filter::NEAREST,
            backend::AddressMode::CLAMP_TO_BORDER,
            backend::AddressMode::CLAMP_TO_BORDER,
            backend::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            backend::MipMapMode::NEAREST));
        samplers.push_back(renderingBackEnd->createSampler(
            backend::Filter::LINEAR,
            backend::Filter::LINEAR,
            backend::AddressMode::CLAMP_TO_BORDER,
            backend::AddressMode::CLAMP_TO_BORDER,
            backend::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            backend::MipMapMode::LINEAR));

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_UBO1, backend::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_UBO2, backend::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURE, backend::DescriptorType::IMAGE, 2);
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, backend::DescriptorType::SAMPLER, 2);
        samplersDescriptorLayout->build();

        pipelineResources["default"] = renderingBackEnd->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            L"default");

        const auto defaultVertexInputLayout = renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes);
        const auto vertexShader = renderingBackEnd->createShaderModule("shaders/myshader_vert");
        const auto fragmentShader = renderingBackEnd->createShaderModule("shaders/myshader_frag");
        pipelines["default"] = renderingBackEnd->createPipeline(
            *pipelineResources["default"],
            *defaultVertexInputLayout,
            *vertexShader,
            *fragmentShader,
            L"default");

        for (uint32_t i = 0; i < backend::SwapChain::FRAMES_IN_FLIGHT; i++) {
            auto descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + std::to_wstring(i));
            auto samplerDescriptorSet = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + std::to_wstring(i));

            descriptorSet->update(BINDING_TEXTURE, textures);
            descriptorSet->update(BINDING_UBO1, uboBuffer1);
            descriptorSet->update(BINDING_UBO2, uboBuffer2);
            samplerDescriptorSet->update(BINDING_SAMPLERS, samplers);

            framesData[i] = renderingBackEnd->createFrameData(i, {descriptorSet, samplerDescriptorSet});
            graphicCommandAllocator[i] = renderingBackEnd->createCommandAllocator(backend::CommandList::GRAPHIC);
            graphicCommandList[i] = graphicCommandAllocator[i]->createCommandList(pipelines["default"]);
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void HelloTriangleApp::onUpdate() {
        const float translationSpeed = 0.005f;
        const float offsetBounds = 1.25f;
        ubo1.offset.x += translationSpeed;
        if (ubo1.offset.x > offsetBounds) {
            ubo1.offset.x = -offsetBounds;
        }
        ubo1.scale += 0.001f * scaleIncrement;
        if ((ubo1.scale > 1.5f) || (ubo1.scale < 0.5f)) {
            scaleIncrement = -scaleIncrement;
        }
        uboBuffer1->write(&ubo1);

        ubo2.color += 0.01f * colorIncrement;
        if ((ubo2.color.x > 0.5f) || (ubo2.color.x < 0.0f)) {
            colorIncrement = -colorIncrement;
        }
        uboBuffer2->write(&ubo2);
    }

    void HelloTriangleApp::onRender() {
        auto swapChain = renderingBackEnd->getSwapChain();
        auto frameData = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frameData)) { return; }

        auto& commandList = graphicCommandList[swapChain->getCurrentFrameIndex()];
        auto pipeline = pipelines["default"];

        commandList->reset();
        commandList->begin(pipeline);
        swapChain->begin(frameData, commandList);
        renderingBackEnd->beginRendering(frameData, pipelineResources["default"], pipeline, commandList);

        commandList->bindVertexBuffer(vertexBuffer);
        commandList->drawInstanced(3);

        renderingBackEnd->endRendering(commandList);
        swapChain->end(frameData, commandList);
        commandList->end();
        renderingBackEnd->getGraphicCommandQueue()->submit(frameData, {commandList});

        swapChain->present(frameData);
        swapChain->nextSwapChain();
    }

    void HelloTriangleApp::onDestroy() {
        uboBuffer1->unmap();
        uboBuffer2->unmap();
        renderingBackEnd->waitIdle();
        for (uint32_t i = 0; i < backend::SwapChain::FRAMES_IN_FLIGHT; i++) {
            renderingBackEnd->destroyFrameData(framesData[i]);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    std::vector<unsigned char> HelloTriangleApp::generateTextureData() const {
        const auto rowPitch = TextureWidth * TexturePixelSize;
        const auto cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
        const auto cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
        const auto textureSize = rowPitch * TextureHeight;

        std::vector<unsigned char> data(textureSize);
        unsigned char* pData = &data[0];

        for (int n = 0; n < textureSize; n += TexturePixelSize) {
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

        return data;
    }

}