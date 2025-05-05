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

        oitDescriptorLayout = vireo->createDescriptorLayout();
        oitDescriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::UNIFORM);
        oitDescriptorLayout->build();

        oitPipelineConfig.depthStencilImageFormat = depthPrepass.getFormat();
        oitPipelineConfig.resources = vireo->createPipelineResources(
            { oitDescriptorLayout, samplerDescriptorLayout },
            pushConstantsDesc);
        oitPipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        oitPipelineConfig.vertexShader = vireo->createShaderModule("shaders/deferred.vert");
        oitPipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_oit.frag");
        oitPipeline = vireo->createGraphicPipeline(oitPipelineConfig);

        compositeDescriptorLayout = vireo->createDescriptorLayout();
        compositeDescriptorLayout->add(BINDING_COLOR_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        compositeDescriptorLayout->add(BINDING_ALPHA_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        compositeDescriptorLayout->build();

        compositePipelineConfig.colorRenderFormats.push_back(renderFormat);
        compositePipelineConfig.resources = vireo->createPipelineResources({ compositeDescriptorLayout, samplerDescriptorLayout });
        compositePipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        compositePipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_oit_composite.frag");
        compositePipeline = vireo->createGraphicPipeline(compositePipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalUniform->map();
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light));
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();
            frame.oitDescriptorSet = vireo->createDescriptorSet(oitDescriptorLayout);
            frame.oitDescriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.oitDescriptorSet->update(BINDING_LIGHT, frame.lightUniform);
            frame.compositeDescriptorSet = vireo->createDescriptorSet(compositeDescriptorLayout);
        }

        samplerDescriptorSet = vireo->createDescriptorSet(samplerDescriptorLayout);
        samplerDescriptorSet->update(BINDING_SAMPLERS, sampler);
    }

    void TransparencyPass::onRender(
        const uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const GBufferPass& gBufferPass,
        const std::shared_ptr<vireo::CommandList>& cmdList,
        const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());

        oitRenderingConfig.colorRenderTargets[BINDING_COLOR_BUFFER].renderTarget = frame.accumColorBuffer;
        oitRenderingConfig.colorRenderTargets[BINDING_ALPHA_BUFFER].renderTarget = frame.accumAlphaBuffer;

        cmdList->barrier(
            {frame.accumColorBuffer, frame.accumAlphaBuffer},
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->setDescriptors({frame.oitDescriptorSet, samplerDescriptorSet});
        cmdList->beginRendering(oitRenderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->bindPipeline(oitPipeline);
        cmdList->bindDescriptors(oitPipeline, {frame.oitDescriptorSet, samplerDescriptorSet});

        pushConstants.modelIndex = Scene::MODEL_TRANSPARENT;
        pushConstants.materialIndex = Scene::MATERIAL_GRID;
        cmdList->pushConstants(oitPipelineConfig.resources, pushConstantsDesc, &pushConstants);
        scene.drawCube(cmdList);

        cmdList->endRendering();
        cmdList->barrier(
            {frame.accumColorBuffer, frame.accumAlphaBuffer},
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);
/*
        frame.descriptorSet->update(BINDING_POSITION_BUFFER, gBufferPass.getPositionBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_NORMAL_BUFFER, gBufferPass.getNormalBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_ALBEDO_BUFFER, gBufferPass.getAlbedoBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_MATERIAL_BUFFER, gBufferPass.getMaterialBuffer(frameIndex)->getImage());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthStencilRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(extent);
        cmdList->setScissors(extent);
        cmdList->setDescriptors({frame.descriptorSet, samplerDescriptorSet});
        cmdList->bindPipeline(pipeline);
        cmdList->setStencilReference(1);
        cmdList->bindDescriptors(pipeline, {frame.descriptorSet, samplerDescriptorSet});
        cmdList->draw(3);
        cmdList->endRendering();*/
    }

    void TransparencyPass::onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList) {
        for (auto& frame : framesData) {
            frame.accumColorBuffer = vireo->createRenderTarget(
                oitPipelineConfig.colorRenderFormats[BINDING_COLOR_BUFFER],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                oitRenderingConfig.colorRenderTargets[BINDING_COLOR_BUFFER].clearValue);
            frame.accumAlphaBuffer = vireo->createRenderTarget(
                oitPipelineConfig.colorRenderFormats[BINDING_ALPHA_BUFFER],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                oitRenderingConfig.colorRenderTargets[BINDING_ALPHA_BUFFER].clearValue);
            cmdList->barrier(
                {frame.accumColorBuffer, frame.accumAlphaBuffer},
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::SHADER_READ);
        }
    }

    void TransparencyPass::onDestroy() {
        for (const auto& frame : framesData) {
            frame.globalUniform->unmap();
        }
    }

}