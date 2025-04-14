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
        renderingBackEnd->setClearColor( 0.0f, 0.2f, 0.4f);
        const auto aspectRatio = renderingBackEnd->getSwapChain()->getAspectRatio();

        const auto triangleVertices = vector<Vertex> {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 0.5f, 0.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f } }
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
            L"CheckerBoardTexture"));
        samplers.push_back(renderingBackEnd->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            0.0f, 1.0f,
            false,
            vireo::MipMapMode::NEAREST));

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandList::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->upload(textures[0], generateTextureData(textures[0]->getWidth(), textures[0]->getHeight()).data());
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::IMAGE, textures.size());
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER, samplers.size());
        samplersDescriptorLayout->build();

        pipelines["default"] = renderingBackEnd->createPipeline(
            renderingBackEnd->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            L"default"),
            renderingBackEnd->createVertexLayout(sizeof(Vertex), vertexAttributes),
            renderingBackEnd->createShaderModule("shaders/triangle_texture.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture.frag"),
            L"default");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            descriptorSet[i] = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + to_wstring(i));
            samplersDescriptorSet[i] = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + to_wstring(i));

            descriptorSet[i]->update(BINDING_TEXTURE, textures);
            samplersDescriptorSet[i]->update(BINDING_SAMPLERS, samplers);

            framesData[i] = renderingBackEnd->createFrameData(i);
            graphicCommandAllocator[i] = renderingBackEnd->createCommandAllocator(vireo::CommandList::GRAPHIC);
            graphicCommandList[i] = graphicCommandAllocator[i]->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TextureApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto frame = swapChain->getCurrentFrameIndex();
        const auto frameData = framesData[frame];

        if (!swapChain->begin(frameData)) { return; }
        graphicCommandAllocator[frame]->reset();
        const auto commandList = graphicCommandList[frame];
        commandList->begin();
        renderingBackEnd->beginRendering(frameData, commandList);

        commandList->bindPipeline(pipelines["default"]);
        commandList->bindDescriptors({descriptorSet[frame], samplersDescriptorSet[frame]});
        commandList->bindVertexBuffer(vertexBuffer);
        commandList->drawInstanced(3);

        renderingBackEnd->endRendering(commandList);
        swapChain->end(frameData, commandList);
        commandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frameData, {commandList});

        swapChain->present(frameData);
        swapChain->nextSwapChain();
    }

    void TextureApp::onDestroy() {
        renderingBackEnd->waitIdle();
        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            renderingBackEnd->destroyFrameData(framesData[i]);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    vector<unsigned char> TextureApp::generateTextureData(uint32_t width, uint32_t height) const {
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