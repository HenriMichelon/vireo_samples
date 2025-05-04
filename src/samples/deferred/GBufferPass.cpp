/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.deferred.gbuffer;

namespace samples {

    void GBufferPass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        sampler = vireo->createSampler(
             vireo::Filter::LINEAR,
             vireo::Filter::LINEAR,
             vireo::AddressMode::CLAMP_TO_BORDER,
             vireo::AddressMode::CLAMP_TO_BORDER,
             vireo::AddressMode::CLAMP_TO_BORDER);

        samplerDescriptorLayout = vireo->createSamplerDescriptorLayout();
        samplerDescriptorLayout->add(BINDING_SAMPLERS, vireo::DescriptorType::SAMPLER);
        samplerDescriptorLayout->build();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_MATERIAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.depthImageFormat = depthPrepass.getFormat();
        pipelineConfig.stencilTestEnable = depthPrepass.isWithStencil();
        pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/deferred_gbuffer.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_gbuffer.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global), 1, L"GLobal");
            frame.globalUniform->map();
            frame.modelUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model), 1, L"Model");
            frame.modelUniform->map();
            frame.materialUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Material), 1, L"Material");
            frame.materialUniform->map();
            frame.materialUniform->write(&scene.getMaterial());
            frame.materialUniform->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"GBuffer");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelUniform);
            frame.descriptorSet->update(BINDING_MATERIAL, frame.materialUniform);
            frame.descriptorSet->update(BINDING_TEXTURES, scene.getTextures());
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLERS, sampler);
    }

    void GBufferPass::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const std::shared_ptr<vireo::CommandList>& cmdList) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());

        renderingConfig.colorRenderTargets[BUFFER_POSITION].renderTarget = frame.positionBuffer;
        renderingConfig.colorRenderTargets[BUFFER_NORMAL].renderTarget = frame.normalBuffer;
        renderingConfig.colorRenderTargets[BUFFER_ALBEDO].renderTarget = frame.albedoBuffer;
        renderingConfig.colorRenderTargets[BUFFER_MATERIAL].renderTarget = frame.materialBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        auto renderTargets = std::views::transform(renderingConfig.colorRenderTargets, [](const auto& colorRenderTarget) {
            return colorRenderTarget.renderTarget;
        });
        cmdList->barrier(
            {renderTargets.begin(), renderTargets.end()},
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->setDescriptors({frame.descriptorSet, samplerDescriptorSet});
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->setStencilReference(1);
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        frame.modelUniform->write(scene.getModels().data());
        scene.drawCube(cmdList);
        cmdList->endRendering();
        cmdList->barrier(
            {renderTargets.begin(), renderTargets.end()},
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);
    }

    void GBufferPass::onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList) {
        for (auto& frame : framesData) {
            frame.positionBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_POSITION],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_POSITION].clearValue);
            frame.normalBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_NORMAL],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_NORMAL].clearValue);
            frame.albedoBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_ALBEDO],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_ALBEDO].clearValue);
            frame.materialBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_MATERIAL],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_MATERIAL].clearValue);
            cmdList->barrier(
                {frame.positionBuffer, frame.normalBuffer, frame.albedoBuffer, frame.materialBuffer},
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::SHADER_READ);
        }
    }

    void GBufferPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.modelUniform->unmap();
            frame.globalUniform->unmap();
        }
    }

}