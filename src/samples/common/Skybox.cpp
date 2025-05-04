/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Libraries.h"
module samples.common.skybox;

namespace samples {

    void Skybox::onUpdate(const Scene& scene) {
        global.view = glm::mat4(glm::mat3(scene.getGlobal().view));
        global.projection = scene.getGlobal().projection;
    }

    void Skybox::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const std::shared_ptr<vireo::CommandList>& uploadCommandList,
        const vireo::ImageFormat renderFormat,
        const DepthPrepass& depthPrepass,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX, sizeof(float) * 3,cubemapVertices.size() / 3);
        uploadCommandList->upload(vertexBuffer, &cubemapVertices[0]);

        sampler = vireo->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER);

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_CUBEMAP, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();
        samplerDescriptorLayout = vireo->createSamplerDescriptorLayout();
        samplerDescriptorLayout->add(BINDING_SAMPLER, vireo::DescriptorType::SAMPLER);
        samplerDescriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.depthImageFormat = depthPrepass.getFormat();
        pipelineConfig.stencilTestEnable = depthPrepass.isWithStencil();
        pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(float) * 3, vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/skybox.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/skybox.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        cubeMap = loadCubemap(uploadCommandList, "res/StandardCubeMap.jpg", vireo::ImageFormat::R8G8B8A8_SRGB);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalBuffer->map();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalBuffer);
            frame.descriptorSet->update(BINDING_CUBEMAP, cubeMap);
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLER, sampler);
    }

    void Skybox::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        bool depthIsReadOnly,
        const DepthPrepass& depthPrepass,
        const std::shared_ptr<vireo::RenderTarget>& colorBuffer,
        const std::shared_ptr<vireo::CommandList>& cmdList) {
        const auto& frame = framesData[frameIndex];

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        frame.globalBuffer->write(&global);

        if (depthIsReadOnly && depthPrepass.isWithStencil()) {
            cmdList->barrier(
                renderingConfig.depthRenderTarget,
                vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL_READ,
                vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL);
        }

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        if (depthPrepass.isWithStencil()) {
            cmdList->setStencilReference(0);
        }
        cmdList->setDescriptors({frame.descriptorSet, samplerDescriptorSet});
        cmdList->bindPipeline(pipeline);
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        cmdList->draw(cubemapVertices.size() / 3);
        cmdList->endRendering();
    }

    void Skybox::onDestroy() {
        for (const auto& frame : framesData) {
            frame.globalBuffer->unmap();
        }
    }

    std::shared_ptr<vireo::Image> Skybox::loadCubemap(
        const std::shared_ptr<vireo::CommandList>& cmdList,
        const std::string& filepath,
        const vireo::ImageFormat imageFormat) const {
        uint32_t texWidth, texHeight;
        uint64_t imageSize;
        auto *pixels = loadRGBAImage(filepath, texWidth, texHeight, imageSize);
        if (!pixels) { throw std::runtime_error("failed to load texture image" + filepath); }

        std::vector<void*> data;
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

    std::byte* Skybox::loadRGBAImage(
        const std::string& filepath,
        uint32_t& width,
        uint32_t& height,
        uint64_t& size) {
        FILE* fp = fopen(filepath.c_str(), "rb");
        if (fp == nullptr) throw std::runtime_error("Error: Could not open file " + filepath);

        int texWidth, texHeight, texChannels;
        unsigned char* imageData = stbi_load_from_file  (
            fp,
            &texWidth, &texHeight,
            &texChannels, STBI_rgb_alpha);
        if (!imageData) throw std::runtime_error("Error loading image : " + std::string{stbi_failure_reason()});

        width = static_cast<uint32_t>(texWidth);
        height = static_cast<uint32_t>(texHeight);
        size = width * height * STBI_rgb_alpha;
        return reinterpret_cast<std::byte*>(imageData);
    }

    std::byte* Skybox::extractImage(
        const std::byte* source,
        const int x, const int y,
        const int srcWidth,
        const int w, const int h,
        const int channels) {
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

}