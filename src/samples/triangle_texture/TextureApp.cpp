/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotexture;

APP(make_shared<samples::TextureApp>(), L"Hello Triangle Texture", 800, 600);

namespace samples {

    void TextureApp::onInit() {
        vertexBuffer = renderingBackEnd->createBuffer(
            vireo::BufferType::VERTEX,
            sizeof(Vertex),
            triangleVertices.size(),
            1,
            L"TriangleVertexBuffer");
        texture = renderingBackEnd->createImage(
            vireo::ImageFormat::R8G8B8A8_SRGB,
            512, 512,
            L"CheckerBoardTexture");
        sampler = renderingBackEnd->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            vireo::MipMapMode::NEAREST);

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->barrier(texture, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        uploadCommandList->upload(texture, generateTextureData(texture->getWidth(), texture->getHeight()).data());
        uploadCommandList->barrier(texture, vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER);
        samplersDescriptorLayout->build();

        defaultPipeline = renderingBackEnd->createGraphicPipeline(
            renderingBackEnd->createPipelineResources(
                { descriptorLayout, samplersDescriptorLayout },
                {},
                L"default"),
            renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes),
            renderingBackEnd->createShaderModule("shaders/triangle_texture.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture.frag"),
            defaultPipelineConfig,
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + to_wstring(i));
            framesData[i].samplersDescriptorSet = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + to_wstring(i));

            framesData[i].descriptorSet->update(BINDING_TEXTURE, texture);
            framesData[i].samplersDescriptorSet->update(BINDING_SAMPLERS, sampler);

            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TextureApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.frameData)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(frame.frameData, swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET);
        cmdList->beginRendering(frame.frameData, swapChain, clearColor);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});
        cmdList->setPrimitiveTopology(vireo::PrimitiveTopology::TRIANGLE_LIST);

        cmdList->bindPipeline(defaultPipeline);
        cmdList->bindDescriptors(defaultPipeline, {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->drawInstanced(triangleVertices.size());

        cmdList->endRendering();
        cmdList->barrier(frame.frameData, swapChain, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::PRESENT);
        cmdList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {cmdList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void TextureApp::onDestroy() {
        renderingBackEnd->waitIdle();
        for (auto& data : framesData) {
            renderingBackEnd->destroyFrameData(data.frameData);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    vector<unsigned char> TextureApp::generateTextureData(const uint32_t width, const uint32_t height) {
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