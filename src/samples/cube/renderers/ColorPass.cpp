/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.cube.colorpass;

namespace samples {

    void ColorPass::onInit(
        const shared_ptr<vireo::Vireo>& vireo,
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
        descriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
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
            frame.lightBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light), 1, L"Light");
            frame.lightBuffer->map();
            frame.lightBuffer->write(&scene.getLight());
            frame.lightBuffer->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"ColorPass");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalBuffer);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelBuffer);
            frame.descriptorSet->update(BINDING_MATERIAL, frame.materialBuffer);
            frame.descriptorSet->update(BINDING_LIGHT, frame.lightBuffer);
            frame.descriptorSet->update(BINDING_TEXTURES, scene.getTextures());
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLERS, sampler);
    }

    void ColorPass::onRender(
       const uint32_t frameIndex,
       const vireo::Extent& extent,
       const Scene& scene,
       const DepthPrepass& depthPrepass,
       const shared_ptr<vireo::CommandList>& cmdList,
       const shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.modelBuffer->write(&scene.getModel());
        frame.globalBuffer->write(&scene.getGlobal());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);
        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        scene.draw(cmdList);
        cmdList->endRendering();
    }

    void ColorPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.modelBuffer->unmap();
            frame.globalBuffer->unmap();
        }
    }




}