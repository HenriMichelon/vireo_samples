/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module samples.deferred.gbuffer;

namespace samples {

    void GBufferPass::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const Samplers& samplers,
        const std::uint32_t framesInFlight) {
        this->vireo = vireo;

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_GLOBAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_MODEL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_MATERIAL, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, scene.getTextures().size());
        descriptorLayout->build();

        pipelineConfig.depthStencilImageFormat = depthPrepass.getFormat();
        pipelineConfig.backStencilOpState = pipelineConfig.frontStencilOpState;
        pipelineConfig.resources = vireo->createPipelineResources(
            { descriptorLayout, samplers.getDescriptorLayout() },
            pushConstantsDesc);
        pipelineConfig.vertexInputLayout = vireo->createVertexLayout(sizeof(Vertex), vertexAttributes);
        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/deferred.vert");
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/deferred_gbuffer.frag");
        pipeline = vireo->createGraphicPipeline(pipelineConfig);

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.commandAllocator = vireo->createCommandAllocator(vireo::CommandType::GRAPHIC);
            frame.commandList = frame.commandAllocator->createCommandList();
            frame.globalUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Global));
            frame.globalUniform->map();
            frame.modelUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Model) * scene.getModels().size());
            frame.modelUniform->map();
            frame.materialUniform = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(Material) * scene.getMaterials().size());
            frame.materialUniform->map();
            frame.materialUniform->write(scene.getMaterials().data());
            frame.materialUniform->unmap();
            frame.descriptorSet = vireo->createDescriptorSet(descriptorLayout, "GBuffer");
            frame.descriptorSet->update(BINDING_GLOBAL, frame.globalUniform);
            frame.descriptorSet->update(BINDING_MODEL, frame.modelUniform, false);
            frame.descriptorSet->update(BINDING_MATERIAL, frame.materialUniform, false);
            frame.descriptorSet->update(BINDING_TEXTURES, scene.getTextures());
        }
    }

    std::shared_ptr<vireo::QueryPool> GBufferPass::onRender(
        const std::uint32_t frameIndex,
        const vireo::Extent& extent,
        const Scene& scene,
        const DepthPrepass& depthPrepass,
        const Samplers& samplers,
        const std::shared_ptr<vireo::Semaphore>& semaphore,
        const std::shared_ptr<vireo::SubmitQueue>& graphicQueue) {
        const auto& frame = framesData[frameIndex];

        frame.globalUniform->write(&scene.getGlobal());
        frame.modelUniform->write(scene.getModels().data());

        renderingConfig.colorRenderTargets[BUFFER_POSITION].renderTarget = frame.positionBuffer;
        renderingConfig.colorRenderTargets[BUFFER_NORMAL].renderTarget = frame.normalBuffer;
        renderingConfig.colorRenderTargets[BUFFER_ALBEDO].renderTarget = frame.albedoBuffer;
        renderingConfig.colorRenderTargets[BUFFER_MATERIAL].renderTarget = frame.materialBuffer;
        renderingConfig.colorRenderTargets[BUFFER_VELOCITY].renderTarget = frame.velocityBuffer;
        renderingConfig.depthStencilRenderTarget = depthPrepass.getDepthBuffer(frameIndex);

        frame.commandAllocator->reset();
        const auto cmdList = frame.commandList;
        cmdList->begin();

        auto renderTargets = std::views::transform(renderingConfig.colorRenderTargets, [](const auto& colorRenderTarget) {
            return colorRenderTarget.renderTarget;
        });
        const std::vector<std::shared_ptr<const vireo::RenderTarget>> images = {renderTargets.begin(), renderTargets.end()};
        cmdList->barrier(
            images,
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);

        // auto pool = vireo->createQueryPool(2, "GBuffer timing");
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

        pushConstants.modelIndex = Scene::MODEL_OPAQUE;
        pushConstants.materialIndex = Scene::MATERIAL_ROCKS;
        cmdList->pushConstants(pipelineConfig.resources, pushConstantsDesc, &pushConstants);
        scene.drawCube(cmdList);

        cmdList->endRendering();
        // cmdList->writeTimestamp(*pool, 1);
        // cmdList->resolveQueryPool(*pool, 0, 2);

        cmdList->barrier(
            images,
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);

        cmdList->end();
        graphicQueue->submit(
           semaphore,
           vireo::WaitStage::VERTEX_SHADER,
           vireo::WaitStage::FRAGMENT_SHADER,
           semaphore,
           {cmdList});
        // return pool;
        return nullptr;
    }

    void GBufferPass::onResize(const vireo::Extent& extent, const std::shared_ptr<vireo::CommandList>& cmdList) {
        for (auto& frame : framesData) {
            frame.positionBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_POSITION],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_POSITION].clearValue,
                1, vireo::MSAA::NONE,
                "Position Buffer");
            frame.normalBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_NORMAL],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_NORMAL].clearValue,
                1, vireo::MSAA::NONE,
                "Normal Buffer");
            frame.albedoBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_ALBEDO],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_ALBEDO].clearValue,
                1, vireo::MSAA::NONE,
                "Albedo Buffer");
            frame.materialBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_MATERIAL],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_MATERIAL].clearValue,
                1, vireo::MSAA::NONE,
                "Material Buffer");
            frame.velocityBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[BUFFER_VELOCITY],
                extent.width,extent.height,
                vireo::RenderTargetType::COLOR,
                renderingConfig.colorRenderTargets[BUFFER_VELOCITY].clearValue,
                1, vireo::MSAA::NONE,
                "Velocity Buffer");
            cmdList->barrier(
                {frame.positionBuffer, frame.normalBuffer, frame.albedoBuffer, frame.materialBuffer, frame.velocityBuffer},
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::SHADER_READ);
        }
    }

}