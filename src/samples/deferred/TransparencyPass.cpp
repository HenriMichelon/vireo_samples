/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
module samples.deferred.oitpass;

namespace samples {

    void TransparencyPass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const vireo::ImageFormat renderFormat,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const Samplers& samplers,
        const uint32_t framesInFlight) {
        this->vireo = vireo;

        oitDescriptorLayout = vireo->createDescriptorLayout();
        oitDescriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->add(BINDING_MATERIAL, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        oitDescriptorLayout->build();

        oitPipelineConfig.depthStencilImageFormat = depthPrepass.getFormat();
        oitPipelineConfig.resources = vireo->createPipelineResources(
            { oitDescriptorLayout, samplers.getDescriptorLayout() },
            pushConstantsDesc);
        oitPipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        oitPipelineConfig.vertexShader = vireo->createShaderModule("shaders/deferred.vert");
        oitPipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_oit.frag");
        oitPipeline = vireo->createGraphicPipeline(oitPipelineConfig);

        compositeDescriptorLayout = vireo->createDescriptorLayout();
        compositeDescriptorLayout->add(BINDING_ACCUM_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        compositeDescriptorLayout->add(BINDING_REVEALAGE_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        compositeDescriptorLayout->build();

        compositePipelineConfig.colorRenderFormats.push_back(renderFormat);
        compositePipelineConfig.resources = vireo->createPipelineResources(
            { compositeDescriptorLayout, samplers.getDescriptorLayout() });
        compositePipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        compositePipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_oit_composite.frag");
        compositePipeline = vireo->createGraphicPipeline(compositePipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalUniform->map();
            frame.modelUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model) * scene.getModels().size());
            frame.modelUniform->map();
            frame.materialUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Material) * scene.getMaterials().size());
            frame.materialUniform->map();
            frame.materialUniform->write(scene.getMaterials().data());
            frame.materialUniform->unmap();
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light));
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();
            frame.oitDescriptorSet = vireo->createDescriptorSet(oitDescriptorLayout);
            frame.oitDescriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.oitDescriptorSet->update(BINDING_MODEL, frame.modelUniform);
            frame.oitDescriptorSet->update(BINDING_LIGHT, frame.lightUniform);
            frame.oitDescriptorSet->update(BINDING_MATERIAL, frame.materialUniform);
            frame.oitDescriptorSet->update(BINDING_TEXTURES, scene.getTextures());
            frame.compositeDescriptorSet = vireo->createDescriptorSet(compositeDescriptorLayout);
        }
    }

    void TransparencyPass::onRender(
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

        oitRenderingConfig.colorRenderTargets[BINDING_ACCUM_BUFFER].renderTarget = frame.accumBuffer;
        oitRenderingConfig.colorRenderTargets[BINDING_REVEALAGE_BUFFER].renderTarget = frame.revealageBuffer;
        oitRenderingConfig.depthStencilRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        cmdList->barrier(
            {frame.accumBuffer, frame.revealageBuffer},
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->setDescriptors({frame.oitDescriptorSet, samplers.getDescriptorSet()});
        cmdList->beginRendering(oitRenderingConfig);
        cmdList->setViewport(vireo::Viewport{
            .width  = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height)});
        cmdList->setScissors(vireo::Rect{
            .width  = extent.width,
            .height = extent.height});
        cmdList->bindPipeline(oitPipeline);
        cmdList->bindDescriptors(oitPipeline, {frame.oitDescriptorSet, samplers.getDescriptorSet()});

        pushConstants.modelIndex = Scene::MODEL_TRANSPARENT;
        pushConstants.materialIndex = Scene::MATERIAL_GRID;
        cmdList->pushConstants(oitPipelineConfig.resources, pushConstantsDesc, &pushConstants);
        scene.drawCube(cmdList);

        cmdList->endRendering();
        cmdList->barrier(
            {frame.accumBuffer, frame.revealageBuffer},
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);

        frame.compositeDescriptorSet->update(BINDING_ACCUM_BUFFER, frame.accumBuffer->getImage());
        frame.compositeDescriptorSet->update(BINDING_REVEALAGE_BUFFER, frame.revealageBuffer->getImage());

        compositeRenderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;

        cmdList->beginRendering(compositeRenderingConfig);
        cmdList->setViewport(vireo::Viewport{
            .width  = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height)});
        cmdList->setScissors(vireo::Rect{
            .width  = extent.width,
            .height = extent.height});
        cmdList->setDescriptors({frame.compositeDescriptorSet, samplers.getDescriptorSet()});
        cmdList->bindPipeline(compositePipeline);
        cmdList->bindDescriptors(compositePipeline, {frame.compositeDescriptorSet, samplers.getDescriptorSet()});
        cmdList->draw(3);
        cmdList->endRendering();
    }

    void TransparencyPass::onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList) {
        for (auto& frame : framesData) {
            frame.accumBuffer = vireo->createRenderTarget(
                oitPipelineConfig.colorRenderFormats[BINDING_ACCUM_BUFFER],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                oitRenderingConfig.colorRenderTargets[BINDING_ACCUM_BUFFER].clearValue);
            frame.revealageBuffer = vireo->createRenderTarget(
                oitPipelineConfig.colorRenderFormats[BINDING_REVEALAGE_BUFFER],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                oitRenderingConfig.colorRenderTargets[BINDING_REVEALAGE_BUFFER].clearValue);
            cmdList->barrier(
                {frame.accumBuffer, frame.revealageBuffer},
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::SHADER_READ);
        }
    }

}