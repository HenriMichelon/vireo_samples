/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotriangle;

APP(make_shared<samples::TriangleApp>(), L"Hello Triangle", 800, 600);

namespace samples {

    void TriangleApp::onInit() {
        renderingBackEnd->setClearColor( 0.0f, 0.2f, 0.4f);
        const auto aspectRatio = renderingBackEnd->getSwapChain()->getAspectRatio();

        const auto triangleVertices = vector<Vertex> {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        vertexBuffer = renderingBackEnd->createBuffer(
            vireo::Buffer::Type::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");
        textures.push_back(renderingBackEnd->createImage(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            512, 512,
            L"CheckerBoardTexture1"));
        textures.push_back(renderingBackEnd->createImage(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            256, 256,
            L"CheckerBoardTexture2"));

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandList::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->reset();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->upload(textures[0], generateTextureData(512, 512).data());
        uploadCommandList->upload(textures[1], generateTextureData(512,512).data());
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        uboBuffer1 = renderingBackEnd->createBuffer(
            vireo::Buffer::UNIFORM,
            sizeof(GlobalUBO1),
            1, 256,
            L"UBO1");
        uboBuffer1->map();
        uboBuffer2 = renderingBackEnd->createBuffer(
            vireo::Buffer::UNIFORM,
            sizeof(GlobalUBO2),
            1, 256,
            L"UBO2");
        uboBuffer2->map();

        samplers.push_back(renderingBackEnd->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            vireo::MipMapMode::NEAREST));

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_UBO1, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_UBO2, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::IMAGE, textures.size());
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER, samplers.size());
        samplersDescriptorLayout->build();

        pipelineResources["default"] = renderingBackEnd->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            L"default");

        const auto defaultVertexInputLayout = renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes);
        const auto vertexShader = renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer.vert");
        const auto fragmentShader = renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer.frag");
        pipelines["default"] = renderingBackEnd->createPipeline(
            pipelineResources["default"],
            defaultVertexInputLayout,
            vertexShader,
            fragmentShader,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            auto descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + to_wstring(i));
            auto samplerDescriptorSet = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + to_wstring(i));

            descriptorSet->update(BINDING_TEXTURE, textures);
            descriptorSet->update(BINDING_UBO1, uboBuffer1);
            descriptorSet->update(BINDING_UBO2, uboBuffer2);
            samplerDescriptorSet->update(BINDING_SAMPLERS, samplers);

            framesData[i] = renderingBackEnd->createFrameData(i, {descriptorSet, samplerDescriptorSet});
            graphicCommandAllocator[i] = renderingBackEnd->createCommandAllocator(vireo::CommandList::GRAPHIC);
            graphicCommandList[i] = graphicCommandAllocator[i]->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TriangleApp::onUpdate() {
        constexpr float translationSpeed = 0.005f;
        constexpr float offsetBounds = 1.25f;
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

    void TriangleApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto frameData = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->begin(frameData)) { return; }

        const auto commandList = graphicCommandList[swapChain->getCurrentFrameIndex()];
        const auto pipeline = pipelines["default"];

        commandList->reset();
        commandList->begin(pipeline);
        renderingBackEnd->beginRendering(frameData, pipelineResources["default"], pipeline, commandList);

        commandList->bindVertexBuffer(vertexBuffer);
        commandList->drawInstanced(3, 2);

        renderingBackEnd->endRendering(commandList);
        swapChain->end(frameData, commandList);
        commandList->end();
        renderingBackEnd->getGraphicCommandQueue()->submit(frameData, {commandList});

        swapChain->present(frameData);
        swapChain->nextSwapChain();
    }

    void TriangleApp::onDestroy() {
        uboBuffer1->unmap();
        uboBuffer2->unmap();
        renderingBackEnd->waitIdle();
        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            renderingBackEnd->destroyFrameData(framesData[i]);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    vector<unsigned char> TriangleApp::generateTextureData(uint32_t width, uint32_t height) const {
        const auto rowPitch = width * 4;
        const auto cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
        const auto cellHeight = width >> 3;    // The height of a cell in the checkerboard texture.
        const auto textureSize = rowPitch * height;

        vector<unsigned char> data(textureSize);
        unsigned char* pData = &data[0];

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

        return data;
    }

}