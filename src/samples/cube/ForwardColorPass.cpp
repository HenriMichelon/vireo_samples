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
        const Samplers& samplers,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        modelsDescriptorLayout = vireo->createDynamicUniformDescriptorLayout();
        materialsDescriptorLayout = vireo->createDynamicUniformDescriptorLayout();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.depthStencilImageFormat = depthPrepass.getFormat();
        pipelineConfig.resources = vireo->createPipelineResources({
            descriptorLayout, samplers.getDescriptorLayout(), modelsDescriptorLayout, materialsDescriptorLayout });
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/cube_color_mvp.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/cube_color_mvp.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.modelUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model), scene.getModels().size());
            frame.materialUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Material), scene.getMaterials().size());
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light), 1);

            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_LIGHT, frame.lightUniform);
            frame.descriptorSet->update(BINDING_TEXTURES, scene.getTextures());
            frame.modelsDescriptorSet = vireo->createDescriptorSet(modelsDescriptorLayout);
            frame.modelsDescriptorSet->update(frame.modelUniform);
            frame.materialsDescriptorSet = vireo->createDescriptorSet(materialsDescriptorLayout);
            frame.materialsDescriptorSet->update(frame.materialUniform);

            frame.globalUniform->map();
            frame.modelUniform->map();
            frame.materialUniform->map();
            frame.materialUniform->write(scene.getMaterials().data());
            frame.materialUniform->unmap();
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();

        }
    }

    void ColorPass::onRender(
       const uint32_t frameIndex,
       const vireo::Extent& extent,
       const Scene& scene,
       const DepthPrepass& depthPrepass,
       const Samplers& samplers,
       const std::shared_ptr<vireo::CommandList>& cmdList,
       const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());
        frame.modelUniform->write(scene.getModels().data());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthStencilRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->setDescriptors({frame.descriptorSet, samplers.getDescriptorSet()});
        cmdList->bindPipeline(pipeline);
        cmdList->bindDescriptor(pipeline, frame.descriptorSet, SET_GLOBAL);
        cmdList->bindDescriptor(pipeline, samplers.getDescriptorSet(), SET_SAMPLERS);

        cmdList->bindDescriptor(pipeline,
            frame.materialsDescriptorSet,SET_MATERIALS,
            frame.materialUniform->getInstanceSizeAligned() * Scene::MATERIAL_ROCKS);
        cmdList->bindDescriptor(pipeline,
            frame.modelsDescriptorSet, SET_MODELS,
            frame.modelUniform->getInstanceSizeAligned() * Scene::MODEL_OPAQUE);
        scene.drawCube(cmdList);

        cmdList->bindDescriptor(pipeline,
            frame.materialsDescriptorSet, SET_MATERIALS,
            frame.materialUniform->getInstanceSizeAligned() * Scene::MATERIAL_GRID);
        cmdList->bindDescriptor(pipeline,
            frame.modelsDescriptorSet, SET_MODELS,
            frame.modelUniform->getInstanceSizeAligned() * Scene::MODEL_TRANSPARENT);
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