/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.deferred.lightingpass;

namespace samples {

    void LightingPass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const vireo::ImageFormat renderFormat,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        sampler = vireo->createSampler(
             vireo::Filter::NEAREST,
             vireo::Filter::NEAREST,
             vireo::AddressMode::CLAMP_TO_BORDER,
             vireo::AddressMode::CLAMP_TO_BORDER,
             vireo::AddressMode::CLAMP_TO_BORDER);

        samplerDescriptorLayout = vireo->createSamplerDescriptorLayout();
        samplerDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER);
        samplerDescriptorLayout->build();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_POSITION_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_NORMAL_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_ALBEDO_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_MATERIAL_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.depthImageFormat = depthPrepass.getFormat();
        pipelineConfig.stencilTestEnable = depthPrepass.isWithStencil();
        pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_lighting.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global), 1, L"GLobal");
            frame.globalUniform->map();
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light), 1, L"Light");
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"ColorPass");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_LIGHT, frame.lightUniform);
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLERS, sampler);
    }

    void LightingPass::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const GBufferPass& gBufferPass,
        const std::shared_ptr<vireo::CommandList>& cmdList,
        const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());

        frame.descriptorSet->update(BINDING_POSITION_BUFFER, gBufferPass.getPositionBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_NORMAL_BUFFER, gBufferPass.getNormalBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_ALBEDO_BUFFER, gBufferPass.getAlbedoBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_MATERIAL_BUFFER, gBufferPass.getMaterialBuffer(frameIndex)->getImage());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        cmdList->barrier(
           colorBuffer,
           vireo::ResourceState::UNDEFINED,
           vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        cmdList->draw(3);
        cmdList->endRendering();
    }

    void LightingPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.globalUniform->unmap();
        }
    }

}