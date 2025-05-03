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
        const shared_ptr<vireo::Vireo>& vireo,
        const vireo::ImageFormat renderFormat,
        const Scene& scene,
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
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MATERIAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/deferred_gbuffer.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_gbuffer.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global), 1, L"GLobal");
            frame.globalBuffer->map();
            frame.modelBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model), 1, L"Model");
            frame.modelBuffer->map();
            frame.materialBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Material), 1, L"Material");
            frame.materialBuffer->map();
            frame.materialBuffer->write(&scene.getMaterial());
            frame.materialBuffer->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"GBuffer");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelBuffer);
            frame.descriptorSet->update(BINDING_MATERIAL, frame.materialBuffer);
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
        const shared_ptr<vireo::CommandList>& cmdList) {
        const auto& frame = framesData[frameIndex];

        frame.modelBuffer->write(&scene.getModel());
        frame.globalBuffer->write(&scene.getGlobal());

        renderingConfig.colorRenderTargets[BUFFER_POSITION].renderTarget = frame.positionBuffer;
        renderingConfig.colorRenderTargets[BUFFER_NORMAL].renderTarget = frame.normalBuffer;
        renderingConfig.colorRenderTargets[BUFFER_ALBEDO].renderTarget = frame.albedoBuffer;
        renderingConfig.colorRenderTargets[BUFFER_RMA].renderTarget = frame.rmaBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        auto renderTargets = views::transform(renderingConfig.colorRenderTargets, [](const auto& colorRenderTarget) {
            return colorRenderTarget.renderTarget;
        });
        cmdList->barrier(
            {renderTargets.begin(), renderTargets.end()},
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        scene.draw(cmdList);
        cmdList->endRendering();
        cmdList->barrier(
            {renderTargets.begin(), renderTargets.end()},
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);
    }

    void GBufferPass::onResize(const vireo::Extent& extent) {
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
            frame.rmaBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_RMA],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_RMA].clearValue);
        }
    }

    void GBufferPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.modelBuffer->unmap();
            frame.globalBuffer->unmap();
        }
    }

}