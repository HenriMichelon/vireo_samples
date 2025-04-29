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
#include "Libraries.h"
module samples.deferred;

namespace samples {

    void DeferredApp::onUpdate() {
        scene.onUpdate();
        postprocessingParams.time = getCurrentTimeMilliseconds();
        postprocessingParamsBuffer->write(&postprocessingParams);
    }

    void DeferredApp::onKeyDown(const uint32_t key) {
        const auto keyCode = static_cast<Scene::KeyCodes>(key);
        if (keyCode == Scene::KeyCodes::P) {
            applyPostProcessing = !applyPostProcessing;
            return;
        }
        scene.onKeyDown(key);
        skyboxGlobal.view = mat4(mat3(scene.getGlobal().view));
    }

    void DeferredApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC, L"MainQueue");
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats[0],
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        skyboxVertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX, sizeof(float) * 3,skyboxVertices.size() / 3);
        skyboxSampler = vireo->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER);

        const auto uploadCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::TRANSFER);
        const auto uploadCommandList = uploadCommandAllocator->createCommandList();
        uploadCommandList->begin();

        scene.onInit(vireo, uploadCommandList, swapChain->getAspectRatio());

        uploadCommandList->upload(skyboxVertexBuffer, &skyboxVertices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        postprocessingParamsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        postprocessingParamsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->build();

        skyboxDescriptorLayout = vireo->createDescriptorLayout();
        skyboxDescriptorLayout->add(SKYBOX_BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        skyboxDescriptorLayout->add(SKYBOX_BINDING_CUBEMAP, vireo::DescriptorType::SAMPLED_IMAGE);
        skyboxDescriptorLayout->build();
        skyboxSamplerDescriptorLayout = vireo->createSamplerDescriptorLayout();
        skyboxSamplerDescriptorLayout->add(SKYBOX_BINDING_SAMPLER, vireo::DescriptorType::SAMPLER);
        skyboxSamplerDescriptorLayout->build();

        postprocessingDescriptorLayout = vireo->createDescriptorLayout();
        postprocessingDescriptorLayout->add(POSTPROCESSING_BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        postprocessingDescriptorLayout->add(POSTPROCESSING_BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        postprocessingDescriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Scene::Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        skyboxPipelineConfig.resources = vireo->createPipelineResources({ skyboxDescriptorLayout, skyboxSamplerDescriptorLayout });
        skyboxPipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(float) * 3, skyboxVertexAttributes);
        skyboxPipelineConfig.vertexShader = vireo->createShaderModule("shaders/skybox.vert");
        skyboxPipelineConfig.fragmentShader = vireo->createShaderModule("shaders/skybox.frag");
        skyboxPipeline = vireo->createGraphicPipeline(skyboxPipelineConfig);

        postprocessingPipelineConfig.resources = skyboxPipelineConfig.resources;
        postprocessingPipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        postprocessingPipelineConfig.fragmentShader = vireo->createShaderModule("shaders/voronoi.frag");
        postprocessingPipeline = vireo->createGraphicPipeline(postprocessingPipelineConfig);

        skyboxGlobal.view = mat4(mat3(scene.getGlobal().view)); // only keep the rotation
        skyboxGlobal.projection = scene.getGlobal().projection;

        const auto skyboxCommandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
        const auto skyboxCommandList = skyboxCommandAllocator->createCommandList();
        skyboxCommandList->begin();
        skyboxCubeMap = loadCubemap(skyboxCommandList, "res/StandardCubeMap.jpg", vireo::ImageFormat::R8G8B8A8_SRGB);
        skyboxCommandList->end();
        graphicQueue->submit({skyboxCommandList});
        graphicQueue->waitIdle();

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Scene::Model));
            frame.modelBuffer->map();

            frame.globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Scene::Global));
            frame.globalBuffer->map();

            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence(true, L"InFlightFence");

            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelBuffer);

            frame.skyboxGlobalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Scene::Global));
            frame.skyboxGlobalBuffer->map();
            frame.skyboxDescriptorSet = vireo->createDescriptorSet(skyboxDescriptorLayout);
            frame.skyboxDescriptorSet->update(SKYBOX_BINDING_GLOBAL, frame.skyboxGlobalBuffer);
            frame.skyboxDescriptorSet->update(SKYBOX_BINDING_CUBEMAP, skyboxCubeMap);



            frame.postProcessingDescriptorSet = vireo->createDescriptorSet(postprocessingDescriptorLayout);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_PARAMS, postprocessingParamsBuffer);
        }

        depthPrepass.onInit(vireo, swapChain->getFramesInFlight());

        skyboxSamplerDescriptorSet = vireo->createDescriptorSet(skyboxSamplerDescriptorLayout);
        skyboxSamplerDescriptorSet->update(SKYBOX_BINDING_SAMPLER, skyboxSampler);

        transferQueue->waitIdle();
    }

    void DeferredApp::onRender() {
        const auto frameIndex = swapChain->getCurrentFrameIndex();
        const auto& frame = framesData[frameIndex];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }

        frame.modelBuffer->write(&scene.getModel());
        frame.globalBuffer->write(&scene.getGlobal());
        frame.skyboxGlobalBuffer->write(&skyboxGlobal);

        depthPrepass.onRender(
            frameIndex,
            swapChain->getExtent(),
            scene,
            graphicQueue);

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();
        renderingConfig.colorRenderTargets[0].renderTarget = frame.colorBuffer;
        renderingConfig.colorRenderTargets[0].multisampledRenderTarget = frame.msaaColorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);
        renderingConfig.multisampledDepthRenderTarget = depthPrepass.getMsaaDepthBuffer(frameIndex);
        cmdList->barrier(
            {frame.colorBuffer, frame.msaaColorBuffer},
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(swapChain->getExtent());
        cmdList->setScissors(swapChain->getExtent());

        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet});
        scene.draw(cmdList);

        cmdList->bindPipeline(skyboxPipeline);
        cmdList->bindVertexBuffer(skyboxVertexBuffer);
        cmdList->bindDescriptors(skyboxPipeline, {frame.skyboxDescriptorSet, skyboxSamplerDescriptorSet});
        cmdList->draw(skyboxVertices.size() / 3);

        cmdList->endRendering();
        cmdList->barrier(
            { depthPrepass.getMsaaDepthBuffer(frameIndex), depthPrepass.getDepthBuffer(frameIndex) },
            vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL_READ,
            vireo::ResourceState::UNDEFINED);
        cmdList->barrier(
            frame.msaaColorBuffer,
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::UNDEFINED);

        shared_ptr<vireo::RenderTarget> colorRenderTarget;
        if (applyPostProcessing) {
            colorRenderTarget = frame.postProcessingColorBuffer;
            postprocessingRenderingConfig.colorRenderTargets[0].renderTarget = frame.postProcessingColorBuffer;
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.postProcessingColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->beginRendering(postprocessingRenderingConfig);
            cmdList->bindPipeline(postprocessingPipeline);
            cmdList->bindDescriptors(postprocessingPipeline, {frame.postProcessingDescriptorSet, skyboxSamplerDescriptorSet});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                frame.postProcessingColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::UNDEFINED);
        } else {
            colorRenderTarget = frame.colorBuffer;
            cmdList->barrier(
                frame.colorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
        }

        cmdList->barrier(
            swapChain,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST);
        cmdList->copy(colorRenderTarget, swapChain);
        cmdList->barrier(
            swapChain,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::PRESENT);
        cmdList->barrier(
            colorRenderTarget,
            vireo::ResourceState::COPY_SRC,
            vireo::ResourceState::UNDEFINED);
        cmdList->end();

        graphicQueue->submit(
            depthPrepass.getSemaphore(frameIndex),
            vireo::WaitStage::DEPTH_STENCIL_TEST_BEFORE_FRAGMENT_SHADER,
            frame.inFlightFence,
            swapChain,
            {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void DeferredApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        postprocessingParams.imageSize.x = extent.width;
        postprocessingParams.imageSize.y = extent.height;
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearValue);
            frame.msaaColorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearValue,
                pipelineConfig.msaa);
            frame.postProcessingColorBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::R8G8B8A8_SRGB,
                extent.width,
                extent.height);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_INPUT, frame.colorBuffer->getImage());
        }
        depthPrepass.onResize(swapChain->getExtent());
    }

    void DeferredApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        for (const auto& frame : framesData) {
            frame.modelBuffer->unmap();
            frame.globalBuffer->unmap();
            frame.skyboxGlobalBuffer->unmap();
        }
        depthPrepass.onDestroy();
    }

    shared_ptr<vireo::Image> DeferredApp::loadCubemap(
        const shared_ptr<vireo::CommandList>& cmdList,
        const string &filepath,
        const vireo::ImageFormat imageFormat) const {
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
                                     1,
                                     6,
                                     L"Cubemap");

        cmdList->barrier(image, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        cmdList->uploadArray(image, data);
        cmdList->barrier(image, vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);

        for (int i = 0; i < 6; i++) {
            delete[] static_cast<std::byte*>(data[i]);
        }
        stbi_image_free(pixels);
        return image;
    }

    std::byte* DeferredApp::loadRGBAImage(const string& filepath, uint32_t& width, uint32_t& height, uint64_t& size) {
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
        return reinterpret_cast<std::byte*>(imageData);
    }

    std::byte *DeferredApp::extractImage(const std::byte* source,
                                const int   x, const int y,
                                const int   srcWidth,
                                const int   w, const int h,
                                const int   channels) {
        const auto extractedImage = new std::byte[w * h * channels];
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

    float DeferredApp::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        return static_cast<float>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

}