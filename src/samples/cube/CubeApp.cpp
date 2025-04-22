/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Macros.h"
module samples.hellocube;

APP(make_shared<samples::CubeApp>(), L"Hello Cube", 1280, 720);

namespace samples {

    void CubeApp::onUpdate() {
        constexpr  float angle = radians(-1.0);
        model.transform = glm::rotate(model.transform, angle, AXIS_X);
        model.transform = glm::rotate(model.transform, angle, AXIS_Y);
        modelBuffer->write(&model, sizeof(Model));
    }

    void CubeApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormat,
            graphicQueue,
            vireo::PresentMode::VSYNC);
        renderingConfig.swapChain = swapChain;

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());

        skyboxVertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX, sizeof(float) * 3,skyboxVertices.size() / 3);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);
        uploadCommandList->upload(skyboxVertexBuffer, &skyboxVertices[0]);
        skyboxCubeMap = loadCubemap("res/StandardCubeMap.jpg", vireo::ImageFormat::R8G8B8A8_SRGB, uploadCommandList);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        global.view = lookAt(cameraPos, cameraTarget, up);
        global.projection = perspective(radians(75.0f), swapChain->getAspectRatio(), 0.1f, 100.0f);
        globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        globalBuffer->map();
        globalBuffer->write(&global);
        globalBuffer->unmap();

        skyboxGlobal.view = mat4(mat3(global.view));
        skyboxGlobal.projection = global.projection;
        skyboxGlobalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        skyboxGlobalBuffer->map();
        skyboxGlobalBuffer->write(&skyboxGlobal);
        skyboxGlobalBuffer->unmap();

        modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model));
        modelBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->build();

        skyboxDescriptorLayout = vireo->createDescriptorLayout();
        skyboxDescriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        skyboxDescriptorLayout->build();

        pipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ descriptorLayout }),
            vireo->createVertexLayout(sizeof(Vertex), vertexAttributes),
            vireo->createShaderModule("shaders/cube_color_mvp.vert"),
            vireo->createShaderModule("shaders/cube_color_mvp.frag"),
            pipelineConfig);

        skyboxPipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ skyboxDescriptorLayout }),
            vireo->createVertexLayout(sizeof(float) * 3, skyboxVertexAttributes),
            vireo->createShaderModule("shaders/skybox.vert"),
            vireo->createShaderModule("shaders/skybox.frag"),
            skyboxPipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, modelBuffer);
            frame.msaaBuffer = vireo->createRenderTarget(swapChain, renderingConfig.clearColorValue, pipelineConfig.msaa);
            frame.depthBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::D32_SFLOAT,
                swapChain->getExtent().width,
                swapChain->getExtent().height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
            frame.msaaDepthBuffer = vireo->createRenderTarget(
                frame.depthBuffer->getImage()->getFormat(),
                frame.depthBuffer->getImage()->getWidth(),
                frame.depthBuffer->getImage()->getHeight(),
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue,
                pipelineConfig.msaa);
        }

        skyboxDescriptorSet = vireo->createDescriptorSet(skyboxDescriptorLayout);
        skyboxDescriptorSet->update(BINDING_GLOBAL, skyboxGlobalBuffer);

        transferQueue->waitIdle();
        uploadCommandList->cleanup();
    }

    void CubeApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();
        cmdList->barrier(swapChain, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(frame.msaaBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(frame.depthBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
        cmdList->barrier(frame.msaaDepthBuffer, vireo::ResourceState::UNDEFINED, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
        renderingConfig.multisampledColorRenderTarget = frame.msaaBuffer;
        renderingConfig.depthRenderTarget = frame.depthBuffer;
        renderingConfig.multisampledDepthRenderTarget = frame.msaaDepthBuffer;
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewports(1, {swapChain->getExtent()});
        cmdList->setScissors(1, {swapChain->getExtent()});

        cmdList->bindPipeline(pipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindIndexBuffer(indexBuffer);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet});
        cmdList->drawIndexed(cubeIndices.size());

        cmdList->bindPipeline(skyboxPipeline);
        cmdList->bindVertexBuffer(skyboxVertexBuffer);
        cmdList->bindDescriptors(skyboxPipeline, {skyboxDescriptorSet});
        cmdList->draw(skyboxVertices.size() / 3);

        cmdList->endRendering();
        cmdList->barrier(frame.msaaBuffer, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(frame.depthBuffer, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(frame.msaaDepthBuffer, vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL, vireo::ResourceState::UNDEFINED);
        cmdList->barrier(swapChain, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::PRESENT);
        cmdList->end();

        graphicQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextSwapChain();
    }

    void CubeApp::onResize() {
        swapChain->recreate();
        for (auto& frame : framesData) {
            frame.msaaBuffer = vireo->createRenderTarget(swapChain, renderingConfig.clearColorValue, pipelineConfig.msaa);
            frame.depthBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::D32_SFLOAT,
                swapChain->getExtent().width,
                swapChain->getExtent().height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
            frame.msaaDepthBuffer = vireo->createRenderTarget(
                frame.depthBuffer->getImage()->getFormat(),
                frame.depthBuffer->getImage()->getWidth(),
                frame.depthBuffer->getImage()->getHeight(),
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue,
                pipelineConfig.msaa);
        }
    }

    void CubeApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        modelBuffer->unmap();
    }

    shared_ptr<vireo::Image> CubeApp::loadCubemap(
        const string &filepath,
        const vireo::ImageFormat imageFormat,
        const shared_ptr<vireo::CommandList>&cmdTransfer) const {
        uint32_t texWidth, texHeight;
        uint64_t imageSize;
        auto *pixels = loadRGBAImage(filepath, texWidth, texHeight, imageSize);
        if (!pixels) { throw runtime_error("failed to load texture image" + filepath); }

        vector<void*> data;
        const auto imgWidth  = texWidth / 4;
        const auto imgHeight = texHeight / 3;
        // right
        data.push_back(extractImage(pixels,
                                    2 * imgWidth,
                                    1 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        // left
        data.push_back(extractImage(pixels,
                                    0 * imgWidth,
                                    1 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        // top
        data.push_back(extractImage(pixels,
                                    1 * imgWidth,
                                    0 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        // bottom
        data.push_back(extractImage(pixels,
                                    1 * imgWidth,
                                    2 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        // front
        data.push_back(extractImage(pixels,
                                    1 * imgWidth,
                                    1 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        // back
        data.push_back(extractImage(pixels,
                                    3 * imgWidth,
                                    1 * imgHeight,
                                    texWidth,
                                    imgWidth,
                                    imgHeight,
                                    4));
        const auto image = vireo->createImage(
                                     imageFormat,
                                     imgWidth, imgHeight,
                                     6);
        cmdTransfer->upload(image, data);
        for (int i = 0; i < 6; i++) {
            delete[] static_cast<byte*>(data[i]);
        }
        stbi_image_free(pixels);
        return image;
    }

    byte* CubeApp::loadRGBAImage(const string& filepath, uint32_t& width, uint32_t& height, uint64_t& size) {
        FILE* fp = fopen(filepath.c_str(), "rb");
        if (fp == nullptr) throw runtime_error("Error: Could not open file " + filepath);

        int texWidth, texHeight, texChannels;
        unsigned char* imageData = stbi_load_from_file  (
            fp,
            &texWidth, &texHeight,
            &texChannels, STBI_rgb_alpha);
        if (!imageData) throw runtime_error("Error loading image : " + string{stbi_failure_reason()});
        width = static_cast<uint32_t>(texWidth);
        height = static_cast<uint32_t>(texHeight);
        size = width * height * STBI_rgb_alpha;
        return reinterpret_cast<byte*>(imageData);
    }

    byte *CubeApp::extractImage(const byte *source,
                                const int   x, const int y,
                                const int   srcWidth,
                                const int   w, const int h,
                                const int   channels) {
        const auto extractedImage = new byte[w * h * channels];
        for (uint32_t row = 0; row < h; ++row) {
            for (uint32_t col = 0; col < w; ++col) {
                for (uint32_t c = 0; c < channels; ++c) {
                    extractedImage[(row * w + col) * channels + c] = source[((y + row) * srcWidth + (x + col)) *
                        channels + c];
                }
            }
        }
        return extractedImage;
    }
}