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
        const std::shared_ptr<vireo::Vireo>& vireo,
        const vireo::ImageFormat renderFormat,
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
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_MATERIAL, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::BUFFER);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.depthImageFormat = depthPrepass.getFormat();
        pipelineConfig.resources = vireo->createPipelineResources({ descriptorLayout, samplerDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
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
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light), 1, L"Light");
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, L"ColorPass");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelUniform);
            frame.descriptorSet->update(BINDING_MATERIAL, frame.materialUniform);
            frame.descriptorSet->update(BINDING_LIGHT, frame.lightUniform);
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
       const std::shared_ptr<vireo::CommandList>& cmdList,
       const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        cmdList->barrier(
          colorBuffer,
          vireo::ResourceState::UNDEFINED,
          vireo::ResourceState::RENDER_TARGET_COLOR);
        if (depthPrepass.isWithStencil()) {
            cmdList->barrier(
                renderingConfig.depthRenderTarget,
                vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL,
                vireo::ResourceState::RENDER_TARGET_DEPTH_STENCIL_READ);
        } else {
            cmdList->barrier(
                renderingConfig.depthRenderTarget,
                vireo::ResourceState::RENDER_TARGET_DEPTH,
                vireo::ResourceState::RENDER_TARGET_DEPTH_READ);
        }

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        frame.modelUniform->write(&scene.getModel(Scene::MODEL_OPAQUE));
        scene.drawCube(cmdList);
        cmdList->endRendering();
    }

    void ColorPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.modelUniform->unmap();
            frame.globalUniform->unmap();
        }
    }

}