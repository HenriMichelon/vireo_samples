/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Macros.h"
module samples.hellotriangle;

APP(make_shared<samples::TextureBufferApp>(), L"Hello Triangle Texture Buffer PushConstants", 800, 600);

namespace samples {

    void TextureBufferApp::onInit() {
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
        globalUboBuffer = renderingBackEnd->createBuffer(
            vireo::BufferType::UNIFORM,
            sizeof(GlobalUBO),
            1, 256,
            L"UBO1");

        const auto uploadCommandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::TRANSFER);
        auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &triangleVertices[0]);
        uploadCommandList->barrier(textures[0], vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        uploadCommandList->upload(textures[0], generateTextureData(textures[0]->getWidth(), textures[0]->getHeight()).data());
        uploadCommandList->barrier(textures[0], vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);
        uploadCommandList->end();
        renderingBackEnd->getTransferCommandQueue()->submit({uploadCommandList});

        globalUboBuffer->map();

        descriptorLayout = renderingBackEnd->createDescriptorLayout(L"Global");
        descriptorLayout->add(BINDING_UBO, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURE, vireo::DescriptorType::SAMPLED_IMAGE, textures.size());
        descriptorLayout->build();

        samplersDescriptorLayout = renderingBackEnd->createSamplerDescriptorLayout(L"Samplers");
        samplersDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER, samplers.size());
        samplersDescriptorLayout->build();

        pipelinesResources["default"] = renderingBackEnd->createPipelineResources(
            { descriptorLayout, samplersDescriptorLayout },
            pushConstantsDesc,
            L"default");
        const auto defaultVertexInputLayout = renderingBackEnd->createVertexLayout(
            sizeof(Vertex),
            vertexAttributes);

        pipelines["shader1"] = renderingBackEnd->createGraphicPipeline(
            pipelinesResources["default"],
            defaultVertexInputLayout,
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer1.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer1.frag"),
            defaultPipelineConfig,
            L"shader1");

        pipelines["shader2"] = renderingBackEnd->createGraphicPipeline(
            pipelinesResources["default"],
            defaultVertexInputLayout,
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer2.vert"),
            renderingBackEnd->createShaderModule("shaders/triangle_texture_buffer2.frag"),
            defaultPipelineConfig,
            L"shader2");

        for (uint32_t i = 0; i < vireo::SwapChain::FRAMES_IN_FLIGHT; i++) {
            framesData[i].descriptorSet = renderingBackEnd->createDescriptorSet(descriptorLayout, L"Global " + to_wstring(i));
            framesData[i].samplersDescriptorSet = renderingBackEnd->createDescriptorSet(samplersDescriptorLayout, L"Samplers " + to_wstring(i));

            framesData[i].descriptorSet->update(BINDING_UBO, globalUboBuffer);
            framesData[i].descriptorSet->update(BINDING_TEXTURE, textures);
            framesData[i].samplersDescriptorSet->update(BINDING_SAMPLERS, samplers);

            framesData[i].frameData = renderingBackEnd->createFrameData(i);
            framesData[i].commandAllocator = renderingBackEnd->createCommandAllocator(vireo::CommandType::GRAPHIC);
            framesData[i].commandList = framesData[i].commandAllocator->createCommandList();
        }

        renderingBackEnd->getTransferCommandQueue()->waitIdle();
        uploadCommandList->cleanup();
    }

    void TextureBufferApp::onUpdate() {
        constexpr float translationSpeed = 0.005f;
        constexpr float offsetBounds = 1.25f;
        globalUbo.offset.x += translationSpeed;
        if (globalUbo.offset.x > offsetBounds) {
            globalUbo.offset.x = -offsetBounds;
        }
        globalUboBuffer->write(&globalUbo);

        pushConstants.color += 0.005f * colorIncrement;
        if ((pushConstants.color.x > 0.5f) || (pushConstants.color.x < 0.0f)) {
            colorIncrement = -colorIncrement;
        }
    }

    void TextureBufferApp::onRender() {
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

        cmdList->bindPipeline(pipelines["shader1"]);
        cmdList->bindDescriptors(pipelines["shader1"], {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->drawInstanced(triangleVertices.size());

        cmdList->bindPipeline(pipelines["shader2"]);
        cmdList->bindDescriptors(pipelines["shader2"], {frame.descriptorSet, frame.samplersDescriptorSet});
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->pushConstants(pipelinesResources["default"], pushConstantsDesc, &pushConstants);
        cmdList->drawInstanced(triangleVertices.size(), 2);

        cmdList->endRendering();
        cmdList->barrier(frame.frameData, swapChain, vireo::ResourceState::RENDER_TARGET, vireo::ResourceState::PRESENT);
        cmdList->end();

        renderingBackEnd->getGraphicCommandQueue()->submit(frame.frameData, {cmdList});

        swapChain->present(frame.frameData);
        swapChain->nextSwapChain();
    }

    void TextureBufferApp::onDestroy() {
        globalUboBuffer->unmap();
        renderingBackEnd->waitIdle();
        for (auto& data : framesData) {
            renderingBackEnd->destroyFrameData(data.frameData);
        }
    }

    // Generate a simple black and white checkerboard texture.
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTexture/D3D12HelloTexture.cpp
    vector<unsigned char> TextureBufferApp::generateTextureData(const uint32_t width, const uint32_t height) {
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
                pData[n + 3] = 0x00;    // A
            }
        }
        return data;
    }

}