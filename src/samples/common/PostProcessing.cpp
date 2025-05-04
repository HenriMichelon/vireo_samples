/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.common.postprocessing;

namespace samples {

    void PostProcessing::onUpdate() {
        params.time = getCurrentTimeMilliseconds();
        paramsBuffer->write(&params);
    }

    void PostProcessing::onInit(
           const std::shared_ptr<vireo::Vireo>& vireo,
           const vireo::ImageFormat renderFormat,
           const uint32_t framesInFlight) {
        this->vireo = vireo;

        sampler = vireo->createSampler(
            vireo::Filter::NEAREST,
            vireo::Filter::NEAREST,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER,
            vireo::AddressMode::CLAMP_TO_BORDER);

        paramsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        paramsBuffer->map();

        samplerDescriptorLayout = vireo->createSamplerDescriptorLayout();
        samplerDescriptorLayout->add(BINDING_SAMPLER, vireo::DescriptorType::SAMPLER);
        samplerDescriptorLayout->build();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.resources = vireo->createPipelineResources({
            descriptorLayout,
            samplerDescriptorLayout });

        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/fxaa.frag");
        fxaaPipeline = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/voronoi.frag");
        effectPipeline = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/gamma_correction.frag");
        gammaCorrectionPipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.fxaaDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.fxaaDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.effectDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.effectDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.gammaCorrectionDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.gammaCorrectionDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLER, sampler);
    }

    void PostProcessing::onRender(
       const uint32_t frameIndex,
       const vireo::Extent& extent,
       const std::shared_ptr<vireo::CommandList>& cmdList,
       const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.fxaaDescriptorSet->update(BINDING_INPUT, colorBuffer->getImage());
        renderingConfig.colorRenderTargets[0].renderTarget = frame.fxaaColorBuffer;
        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);
        cmdList->barrier(
            frame.fxaaColorBuffer,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(fxaaPipeline);
        cmdList->bindDescriptors(fxaaPipeline, {frame.fxaaDescriptorSet, samplerDescriptorSet});
        cmdList->draw(3);
        cmdList->endRendering();

        if (applyEffect) {
            frame.effectDescriptorSet->update(BINDING_INPUT, frame.fxaaColorBuffer->getImage());
            renderingConfig.colorRenderTargets[0].renderTarget = frame.effectColorBuffer;
            cmdList->barrier(
                frame.fxaaColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.effectColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->beginRendering(renderingConfig);
            cmdList->bindPipeline(effectPipeline);
            cmdList->bindDescriptors(effectPipeline, {frame.effectDescriptorSet, samplerDescriptorSet});
            cmdList->draw(3);
            cmdList->endRendering();
        }

        if (applyGammaCorrection) {
            const auto colorInput = applyEffect ? frame.effectColorBuffer->getImage() : frame.fxaaColorBuffer->getImage();
            frame.gammaCorrectionDescriptorSet->update(BINDING_INPUT, colorInput);
            renderingConfig.colorRenderTargets[0].renderTarget = frame.gammaCorrectionColorBuffer;
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.gammaCorrectionColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->beginRendering(renderingConfig);
            cmdList->bindPipeline(gammaCorrectionPipeline);
            cmdList->bindDescriptors(gammaCorrectionPipeline, {frame.gammaCorrectionDescriptorSet, samplerDescriptorSet});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                frame.gammaCorrectionColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::UNDEFINED);
            if (applyEffect) {
                cmdList->barrier(
                    frame.fxaaColorBuffer,
                    vireo::ResourceState::SHADER_READ,
                    vireo::ResourceState::UNDEFINED);
            }
        } else if (applyEffect) {
            cmdList->barrier(
                frame.fxaaColorBuffer,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::UNDEFINED);
            cmdList->barrier(
                frame.effectColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
        } else {
            cmdList->barrier(
                frame.fxaaColorBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::COPY_SRC);
        }

        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::UNDEFINED);
    }

    void PostProcessing::onResize(const vireo::Extent& extent) {
        params.imageSize.x = extent.width;
        params.imageSize.y = extent.height;
        for (auto& frame : framesData) {
            frame.fxaaColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width,
                extent.height);
            frame.effectColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width,
                extent.height);
            frame.gammaCorrectionColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width,
                extent.height);
        }
    }

    float PostProcessing::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        return static_cast<float>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }
}