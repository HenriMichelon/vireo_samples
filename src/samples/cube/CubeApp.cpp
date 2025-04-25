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

namespace samples {

    void CubeApp::onUpdate() {
        constexpr  float angle = radians(-1.0);
        model.transform = glm::rotate(model.transform, angle, AXIS_X);
        model.transform = glm::rotate(model.transform, angle, AXIS_Y);
        modelBuffer->write(&model, sizeof(Model));

        postprocessingParams.time = getCurrentTimeMilliseconds();
        postprocessingParamsBuffer->write(&postprocessingParams);
    }

    void CubeApp::onKeyDown(const uint32_t key) {
        const auto keyCode = static_cast<KeyCodes>(key);
        if (keyCode == KeyCodes::P) {
            applyPostProcessing = !applyPostProcessing;
            return;
        }
        // cout << "key down: " << key << endl;
        vec3 axis;
        auto angle = radians(2.0f);
        switch (keyCode) {
        case KeyCodes::LEFT:
            axis = AXIS_Y;
            break;
        case KeyCodes::RIGHT:
            axis = AXIS_Y;
            angle *= -1.0f;
            break;
        case KeyCodes::UP:
            if (cameraYRotationAngle <= radians(-60.f)) { return; }
            axis = AXIS_X;
            angle *= -1.0f;
            cameraYRotationAngle += angle;
            break;
        case KeyCodes::DOWN:
            if (cameraYRotationAngle >= radians(60.0f)) { return; }
            axis = AXIS_X;
            cameraYRotationAngle += angle;
            break;
        default:
            return;
        }
        const auto viewDir = cameraTarget - cameraPos;
        const vec3 rotatedDir = rotate(mat4{1.0f}, angle, axis) * vec4(viewDir, 0.0f);
        cameraTarget = cameraPos + rotatedDir;

        global.view = lookAt(cameraPos, cameraTarget, AXIS_Y);
        skyboxGlobal.view = mat4(mat3(global.view));

        graphicQueue->waitIdle();
        globalBuffer->write(&global);
        skyboxGlobalBuffer->write(&skyboxGlobal);
    }

    void CubeApp::onInit() {
        graphicQueue = vireo->createSubmitQueue(vireo::CommandType::GRAPHIC);
        swapChain = vireo->createSwapChain(
            pipelineConfig.colorRenderFormats[0],
            graphicQueue,
            windowHandle,
            vireo::PresentMode::VSYNC);

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());

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
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);
        uploadCommandList->upload(skyboxVertexBuffer, &skyboxVertices[0]);
        uploadCommandList->end();
        const auto transferQueue = vireo->createSubmitQueue(vireo::CommandType::TRANSFER);
        transferQueue->submit({uploadCommandList});

        global.view = lookAt(cameraPos, cameraTarget, up);
        global.projection = perspective(radians(75.0f), swapChain->getAspectRatio(), 0.1f, 100.0f);
        globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        globalBuffer->map();
        globalBuffer->write(&global);

        skyboxGlobal.view = mat4(mat3(global.view)); // only keep the rotation
        skyboxGlobal.projection = global.projection;
        skyboxGlobalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
        skyboxGlobalBuffer->map();
        skyboxGlobalBuffer->write(&skyboxGlobal);

        postprocessingParamsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        postprocessingParamsBuffer->map();

        modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model));
        modelBuffer->map();

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

        pipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ descriptorLayout }),
            vireo->createVertexLayout(sizeof(Vertex), vertexAttributes),
            vireo->createShaderModule("shaders/cube_color_mvp.vert"),
            vireo->createShaderModule("shaders/cube_color_mvp.frag"),
            pipelineConfig);

        skyboxPipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ skyboxDescriptorLayout, skyboxSamplerDescriptorLayout }),
            vireo->createVertexLayout(sizeof(float) * 3, skyboxVertexAttributes),
            vireo->createShaderModule("shaders/skybox.vert"),
            vireo->createShaderModule("shaders/skybox.frag"),
            skyboxPipelineConfig);

        postprocessingPipeline = vireo->createGraphicPipeline(
            vireo->createPipelineResources({ postprocessingDescriptorLayout, skyboxSamplerDescriptorLayout }),
            vireo->createVertexLayout(0, postprocessingAttributes),
            vireo->createShaderModule("shaders/quad.vert"),
            vireo->createShaderModule("shaders/voronoi.frag"),
            postprocessingPipelineConfig);

        framesData.resize(swapChain->getFramesInFlight());
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.inFlightFence =vireo->createFence();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, modelBuffer);

            frame.postProcessingDescriptorSet = vireo->createDescriptorSet(postprocessingDescriptorLayout);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_PARAMS, postprocessingParamsBuffer);
        }

        framesData[0].commandList->begin();
        skyboxCubeMap = loadCubemap(framesData[0].commandList, "res/StandardCubeMap.jpg", vireo::ImageFormat::R8G8B8A8_SRGB);
        framesData[0].commandList->end();
        graphicQueue->submit({framesData[0].commandList});
        graphicQueue->waitIdle();
        framesData[0].commandList->cleanup();

        skyboxDescriptorSet = vireo->createDescriptorSet(skyboxDescriptorLayout);
        skyboxDescriptorSet->update(SKYBOX_BINDING_GLOBAL, skyboxGlobalBuffer);
        skyboxDescriptorSet->update(SKYBOX_BINDING_CUBEMAP, skyboxCubeMap);
        skyboxSamplerDescriptorSet = vireo->createDescriptorSet(skyboxSamplerDescriptorLayout);
        skyboxSamplerDescriptorSet->update(SKYBOX_BINDING_SAMPLER, skyboxSampler);

        transferQueue->waitIdle();
        uploadCommandList->cleanup();
    }

    void CubeApp::onRender() {
        const auto& frame = framesData[swapChain->getCurrentFrameIndex()];

        if (!swapChain->acquire(frame.inFlightFence)) { return; }
        frame.commandAllocator->reset();

        const auto& cmdList = frame.commandList;
        cmdList->begin();

        renderingConfig.colorRenderTargets[0].renderTarget = frame.colorBuffer;
        renderingConfig.colorRenderTargets[0].multisampledRenderTarget = frame.msaaColorBuffer;
        renderingConfig.depthRenderTarget = frame.depthBuffer;
        renderingConfig.multisampledDepthRenderTarget = frame.msaaDepthBuffer;
        cmdList->barrier(
            {frame.colorBuffer, frame.msaaColorBuffer},
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(
            {frame.depthBuffer, frame.msaaDepthBuffer},
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
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
        cmdList->bindDescriptors(skyboxPipeline, {skyboxDescriptorSet, skyboxSamplerDescriptorSet});
        cmdList->draw(skyboxVertices.size() / 3);

        cmdList->endRendering();
        cmdList->barrier(
            { frame.msaaDepthBuffer, frame.depthBuffer },
            vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL,
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

        graphicQueue->submit(frame.inFlightFence, swapChain, {cmdList});
        swapChain->present();
        swapChain->nextFrameIndex();
    }

    void CubeApp::onResize() {
        swapChain->recreate();
        const auto extent = swapChain->getExtent();
        postprocessingParams.imageSize.x = extent.width;
        postprocessingParams.imageSize.y = extent.height;
        for (auto& frame : framesData) {
            frame.colorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearColorValue);
            frame.msaaColorBuffer = vireo->createRenderTarget(
                swapChain,
                renderingConfig.colorRenderTargets[0].clearColorValue,
                pipelineConfig.msaa);
            frame.depthBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::D32_SFLOAT,
                extent.width,
                extent.height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue);
            frame.msaaDepthBuffer = vireo->createRenderTarget(
                frame.depthBuffer->getImage()->getFormat(),
                extent.width,
                extent.height,
                vireo::RenderTargetType::DEPTH,
                renderingConfig.depthClearValue,
                pipelineConfig.msaa);
            frame.postProcessingColorBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::R8G8B8A8_SRGB,
                extent.width,
                extent.height);
            frame.postProcessingDescriptorSet->update(POSTPROCESSING_BINDING_INPUT, frame.colorBuffer->getImage());
        }
    }

    void CubeApp::onDestroy() {
        graphicQueue->waitIdle();
        swapChain->waitIdle();
        modelBuffer->unmap();
        globalBuffer->unmap();
        skyboxGlobalBuffer->unmap();
    }

    shared_ptr<vireo::Image> CubeApp::loadCubemap(
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

    std::byte* CubeApp::loadRGBAImage(const string& filepath, uint32_t& width, uint32_t& height, uint64_t& size) {
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

    std::byte *CubeApp::extractImage(const std::byte* source,
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

    float CubeApp::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        return static_cast<float>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

}