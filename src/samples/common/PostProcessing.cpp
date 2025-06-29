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

    void PostProcessing::onKeyDown(const KeyScanCodes keyCode) {
        switch (keyCode) {
        case KeyScanCodes::F:
            toggleFXAA();
            std::cout << "FXAA: " << (applyFXAA ? "ON" : "OFF") << std::endl;
            break;
        case KeyScanCodes::M:
            toggleSMAA();
            std::cout << "SMAA: " << (applySMAA ? "ON" : "OFF") << std::endl;
            break;
        case KeyScanCodes::P:
            toggleDisplayEffect();
            std::cout << "Effect: " << (applyEffect ? "ON" : "OFF") << std::endl;
            break;
        case KeyScanCodes::G:
            toggleGammaCorrection();
            std::cout << "Gamma Correction: " << (applyGammaCorrection ? "ON" : "OFF") << std::endl;
            break;
        default:
            break;
        }
    }

    void PostProcessing::onInit(
           const std::shared_ptr<vireo::Vireo>& vireo,
           const vireo::ImageFormat renderFormat,
           const Samplers& samplers,
           const uint32_t framesInFlight) {
        this->vireo = vireo;

        paramsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        paramsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        smaaDescriptorLayout = vireo->createDescriptorLayout();
        smaaDescriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::UNIFORM);
        smaaDescriptorLayout->add(BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        smaaDescriptorLayout->add(BINDING_SMAA_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        smaaDescriptorLayout->build();

        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");

        auto resources = vireo->createPipelineResources({
            descriptorLayout,
            samplers.getDescriptorLayout() });
        auto smaaResources = vireo->createPipelineResources({
            smaaDescriptorLayout,
            samplers.getDescriptorLayout() });

        pipelineConfig.colorRenderFormats.push_back(vireo::ImageFormat::R16G16_SFLOAT);
        pipelineConfig.resources = resources;
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/smaa_edge_detect.frag");
        smaaEdgePipeline = vireo->createGraphicPipeline(pipelineConfig);
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/smaa_blend_weigth.frag");
        smaaBlendWeightPipeline = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.colorRenderFormats[0] = renderFormat;
        pipelineConfig.resources = smaaResources;
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/smaa_neighborhood_blend.frag");
        smaaBlendPipeline = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.resources = resources;
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
            frame.smaaEdgeDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.smaaEdgeDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.smaaBlendWeightDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.smaaBlendWeightDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.smaaBlendDescriptorSet = vireo->createDescriptorSet(smaaDescriptorLayout);
            frame.smaaBlendDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
        }
    }

    void PostProcessing::onRender(
       const uint32_t frameIndex,
       const vireo::Extent& extent,
       const Samplers& samplers,
       const std::shared_ptr<vireo::CommandList>& cmdList,
       const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];
        std::vector<std::shared_ptr<vireo::Image>> shaderReadTargets;

        if (applySMAA) {
            cmdList->barrier(
               colorBuffer,
               vireo::ResourceState::RENDER_TARGET_COLOR,
               vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.smaaEdgeBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            frame.smaaEdgeDescriptorSet->update(BINDING_INPUT, colorBuffer->getImage());
            renderingConfig.colorRenderTargets[0].renderTarget = frame.smaaEdgeBuffer;
            cmdList->beginRendering(renderingConfig);
            cmdList->setViewport(vireo::Viewport{
                .width  = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height)});
            cmdList->setScissors(vireo::Rect{
                .width  = extent.width,
                .height = extent.height});
            cmdList->bindPipeline(smaaEdgePipeline);
            cmdList->bindDescriptors({frame.smaaEdgeDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(colorBuffer->getImage());

            cmdList->barrier(
                frame.smaaEdgeBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.smaaBlendBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            frame.smaaBlendWeightDescriptorSet->update(BINDING_INPUT, frame.smaaEdgeBuffer->getImage());
            renderingConfig.colorRenderTargets[0].renderTarget = frame.smaaBlendBuffer;
            cmdList->beginRendering(renderingConfig);
            cmdList->setViewport(vireo::Viewport{
                .width  = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height)});
            cmdList->setScissors(vireo::Rect{
                .width  = extent.width,
                .height = extent.height});
            cmdList->bindPipeline(smaaBlendWeightPipeline);
            cmdList->bindDescriptors({frame.smaaBlendWeightDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(frame.smaaEdgeBuffer->getImage());

            cmdList->barrier(
                frame.smaaBlendBuffer,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.smaaColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            frame.smaaBlendDescriptorSet->update(BINDING_INPUT, colorBuffer->getImage());
            frame.smaaBlendDescriptorSet->update(BINDING_SMAA_INPUT, frame.smaaBlendBuffer->getImage());
            renderingConfig.colorRenderTargets[0].renderTarget = frame.smaaColorBuffer;
            cmdList->beginRendering(renderingConfig);
            cmdList->setViewport(vireo::Viewport{
                .width  = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height)});
            cmdList->setScissors(vireo::Rect{
                .width  = extent.width,
                .height = extent.height});
            cmdList->bindPipeline(smaaBlendPipeline);
            cmdList->bindDescriptors({frame.smaaBlendDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(frame.smaaBlendBuffer->getImage());
        }

        if (applyFXAA) {
            const auto colorInput =
                applySMAA ? frame.smaaColorBuffer->getImage() :
                colorBuffer->getImage();
            cmdList->barrier(
               colorInput,
               vireo::ResourceState::RENDER_TARGET_COLOR,
               vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.fxaaColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            frame.fxaaDescriptorSet->update(BINDING_INPUT, colorBuffer->getImage());
            renderingConfig.colorRenderTargets[0].renderTarget = frame.fxaaColorBuffer;
            cmdList->beginRendering(renderingConfig);
            cmdList->setViewport(vireo::Viewport{
                .width  = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height)});
            cmdList->setScissors(vireo::Rect{
                .width  = extent.width,
                .height = extent.height});
            cmdList->bindPipeline(fxaaPipeline);
            cmdList->bindDescriptors({frame.fxaaDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(colorInput);
        }

        if (applyEffect) {
            const auto colorInput =
                applyFXAA ? frame.fxaaColorBuffer->getImage() :
                applySMAA ? frame.smaaColorBuffer->getImage() :
                colorBuffer->getImage();
            frame.effectDescriptorSet->update(BINDING_INPUT, colorInput);
            renderingConfig.colorRenderTargets[0].renderTarget = frame.effectColorBuffer;
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->barrier(
                frame.effectColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->beginRendering(renderingConfig);
            cmdList->bindPipeline(effectPipeline);
            cmdList->bindDescriptors({frame.effectDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(colorInput);
        }

        if (applyGammaCorrection) {
            const auto colorInput =
                applyEffect ? frame.effectColorBuffer->getImage() :
                applyFXAA ? frame.fxaaColorBuffer->getImage():
                applySMAA ? frame.smaaColorBuffer->getImage():
                colorBuffer->getImage();
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
            cmdList->bindDescriptors({frame.gammaCorrectionDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            shaderReadTargets.push_back(colorInput);
        }

        for (const auto& image : shaderReadTargets) {
            cmdList->barrier(
                image,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::UNDEFINED);
        }
    }

    void PostProcessing::onResize(const vireo::Extent& extent) {
        params.imageSize.x = extent.width;
        params.imageSize.y = extent.height;
        for (auto& frame : framesData) {
            frame.fxaaColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width,
                extent.height);
            frame.smaaColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width,
                extent.height);
            frame.smaaEdgeBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::R16G16_SFLOAT,
                extent.width,
                extent.height);
            frame.smaaBlendBuffer = vireo->createRenderTarget(
                vireo::ImageFormat::R16G16_SFLOAT,
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