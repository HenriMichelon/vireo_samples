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

        triangleVertices = {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        vertexBuffer = renderingBackEnd->createBuffer(
            vireo::BufferType::VERTEX,
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
        uboBuffer1 = renderingBackEnd->createBuffer(
            vireo::BufferType::UNIFORM,
            sizeof(GlobalUBO1),
            1, 256,
            L"UBO1");
        uboBuffer2 = renderingBackEnd->createBuffer(
            vireo::BufferType::UNIFORM,
            sizeof(GlobalUBO2),
            1, 256,
            L"UBO2");

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->upload(textures[0], generateTextureData(textures[0]->getWidth(), textures[0]->getHeight()).data());
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        uboBuffer1->map();
        uboBuffer2->map();

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_UBO1, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_UBO2, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::IMAGE, textures.size());
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER, samplers.size());
        samplersDescriptorLayout->build();

        const auto defaultLayout = renderingBackEnd->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            L"default");
        const auto defaultVertexInputLayout = renderingBackEnd->createVertexLayout(
            sizeof(Vertex),
            vertexAttributes);

        pipelines["shader1"] = renderingBackEnd->createPipeline(
            defaultLayout,
            defaultVertexInputLayout,
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer1.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer1.frag"),
            vireo::CullMode::BACK,
            L"shader1");

        pipelines["shader2"] = renderingBackEnd->createPipeline(
            defaultLayout,
            defaultVertexInputLayout,
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer2.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer2.frag"),
            vireo::CullMode::BACK,
            L"shader2");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + to_wstring(i));
            framesData[i].samplersDescriptorSet = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + to_wstring(i));

            framesData[i].descriptorSet->update(BINDING_UBO1, uboBuffer1);
            framesData[i].descriptorSet->update(BINDING_UBO2, uboBuffer2);
            framesData[i].descriptorSet->update(BINDING_TEXTURE, textures);
            framesData[i].samplersDescriptorSet->update(BINDING_SAMPLERS, samplers);

            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
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

        ubo2.color += 0.005f * colorIncrement;
        if ((ubo2.color.x > 0.5f) || (ubo2.color.x < 0.0f)) {
            colorIncrement = -colorIncrement;
        }
        uboBuffer2->write(&ubo2);
    }

    void TriangleApp::onRender() {
        const auto swapChain = renderingBackEnd->getSwapChain();
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->begin(frame.frameData)) { return; }
        frame.commandAllocator->reset();
        frame.commandList->begin();
        renderingBackEnd->beginRendering(frame.frameData, frame.commandList);

        frame.commandList->bindPipeline(pipelines["shader1"]);
        frame.commandList->bindDescriptors({frame.descriptorSet, frame.samplersDescriptorSet});
        frame.commandList->bindVertexBuffer(vertexBuffer);
        frame.commandList->drawInstanced(triangleVertices.size());

        frame.commandList->bindPipeline(pipelines["shader2"]);
        frame.commandList->bindDescriptors({frame.descriptorSet, frame.samplersDescriptorSet});
        frame.commandList->bindVertexBuffer(vertexBuffer);
        frame.commandList->drawInstanced(triangleVertices.size(), 2);

        renderingBackEnd->endRendering(frame.commandList);
        swapChain->end(frame.frameData, frame.commandList);
        frame.commandList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {frame.commandList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void TriangleApp::onDestroy() {
        uboBuffer1->unmap();
        uboBuffer2->unmap();
        renderingBackEnd->waitIdle();
        for (auto& data : framesData) {
            renderingBackEnd->destroyFrameData(data.frameData);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    vector<unsigned char> TriangleApp::generateTextureData(const uint32_t width, const uint32_t height) {
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