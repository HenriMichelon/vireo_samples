/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module samples.deferred.lightingpass;

namespace samples {

    void LightingPass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const vireo::ImageFormat renderFormat,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const Samplers& samplers,
        const std::uint32_t framesInFlight) {
        this->vireo = vireo;

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_LIGHT, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_POSITION_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_NORMAL_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_ALBEDO_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->add(BINDING_MATERIAL_BUFFER, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.depthStencilImageFormat = depthPrepass.getFormat();
        pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        pipelineConfig.resources = vireo->createPipelineResources(
            { descriptorLayout, samplers.getDescriptorLayout() });
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_lighting.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalUniform->map();
            frame.lightUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Light));
            frame.lightUniform->map();
            auto light = scene.getLight();
            frame.lightUniform->write(&light);
            frame.lightUniform->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_LIGHT, frame.lightUniform);
        }
    }

    std::shared_ptr<vireo::QueryPool> LightingPass::onRender(
        const std::uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const GBufferPass& gBufferPass,
        const Samplers& samplers,
        const std::shared_ptr<vireo::CommandList>& cmdList,
        const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());

        frame.descriptorSet->update(BINDING_POSITION_BUFFER, gBufferPass.getPositionBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_NORMAL_BUFFER, gBufferPass.getNormalBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_ALBEDO_BUFFER, gBufferPass.getAlbedoBuffer(frameIndex)->getImage());
        frame.descriptorSet->update(BINDING_MATERIAL_BUFFER, gBufferPass.getMaterialBuffer(frameIndex)->getImage());

        renderingConfig.colorRenderTargets[0].renderTarget = colorBuffer;
        renderingConfig.depthStencilRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        // auto pool = vireo->createQueryPool(2, "Lighting timing");
        // cmdList->writeTimestamp(*pool, 0);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(vireo::Viewport{
            static_cast<float>(extent.width),
            static_cast<float>(extent.height)});
        cmdList->setScissors(vireo::Rect{
            extent.width,
            extent.height});
        cmdList->bindPipeline(pipeline);
        cmdList->setStencilReference(1);
        cmdList->bindDescriptors({frame.descriptorSet, samplers.getDescriptorSet()});
        cmdList->draw(3);
        cmdList->endRendering();

        // cmdList->writeTimestamp(*pool, 1);
        // cmdList->resolveQueryPool(*pool, 0, 2);
        // return pool;
        return nullptr;
    }

}