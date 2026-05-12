/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
module samples.common.postprocessing;

namespace samples {

    void PostProcessing::onUpdate() {
        params.time = getCurrentTimeMilliseconds();
        paramsBuffer->write(&params);
    }

    void PostProcessing::onInit(
           const std::shared_ptr<vireo::Vireo>& vireo,
           const vireo::ImageFormat renderFormat,
           const Samplers& samplers,
           const std::uint32_t framesInFlight) {
        this->vireo = vireo;

        paramsBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM,sizeof(PostProcessingParams));
        paramsBuffer->map();

        descriptorLayout = vireo->createDescriptorLayout();
        descriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::UNIFORM);
        descriptorLayout->add(BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        descriptorLayout->build();

        taaDescriptorLayout = vireo->createDescriptorLayout();
        taaDescriptorLayout->add(BINDING_PARAMS, vireo::DescriptorType::UNIFORM);
        taaDescriptorLayout->add(BINDING_INPUT, vireo::DescriptorType::SAMPLED_IMAGE);
        taaDescriptorLayout->add(BINDING_HISTORY, vireo::DescriptorType::SAMPLED_IMAGE);
        taaDescriptorLayout->add(BINDING_VELOCITY, vireo::DescriptorType::SAMPLED_IMAGE);
        taaDescriptorLayout->build();

        pipelineConfig.vertexShader = vireo->createShaderModule("shaders/quad.vert");

        const auto resources = vireo->createPipelineResources({
            descriptorLayout,
            samplers.getDescriptorLayout() });
        const auto taaResources = vireo->createPipelineResources({
            taaDescriptorLayout,
            samplers.getDescriptorLayout() });

        pipelineConfig.colorRenderFormats.push_back(renderFormat);
        pipelineConfig.resources = taaResources;
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/taa.frag");
        taaPipeline = vireo->createGraphicPipeline(pipelineConfig);

        pipelineConfig.resources = resources;
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/fxaa.frag");
        fxaaPipeline = vireo->createGraphicPipeline(pipelineConfig);
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/voronoi.frag");
        effectPipeline = vireo->createGraphicPipeline(pipelineConfig);
        pipelineConfig.fragmentShader = vireo->createShaderModule("shaders/gamma_correction.frag");
        gammaCorrectionPipeline = vireo->createGraphicPipeline(pipelineConfig);

        // SMAA compute pipelines
        smaaDataBuffer = vireo->createBuffer(vireo::BufferType::UNIFORM, sizeof(SmaaData));
        smaaDataBuffer->map();
        smaaDataBuffer->write(&smaaData);

        smaaComputeDescLayout = vireo->createDescriptorLayout();
        smaaComputeDescLayout->add(BINDING_PARAMS,       vireo::DescriptorType::UNIFORM);
        smaaComputeDescLayout->add(SMAA_BINDING_DATA,    vireo::DescriptorType::UNIFORM);
        smaaComputeDescLayout->add(SMAA_BINDING_OUTPUT,  vireo::DescriptorType::READWRITE_IMAGE);
        smaaComputeDescLayout->add(SMAA_BINDING_TEXTURES, vireo::DescriptorType::SAMPLED_IMAGE, SMAA_TEXTURES_COUNT);
        smaaComputeDescLayout->build();

        smaaExtraDescLayout = vireo->createDescriptorLayout();
        smaaExtraDescLayout->add(SMAA_BINDING_EDGE,   vireo::DescriptorType::READWRITE_IMAGE);
        smaaExtraDescLayout->add(SMAA_BINDING_WEIGHT, vireo::DescriptorType::READWRITE_IMAGE);
        smaaExtraDescLayout->build();

        smaaComputeResources = vireo->createPipelineResources({
            smaaComputeDescLayout,
            samplers.getDescriptorLayout(),
            smaaExtraDescLayout });

        smaaEdgePipeline        = vireo->createComputePipeline(smaaComputeResources, vireo->createShaderModule("shaders/smaa_edge_detect.comp"));
        smaaBlendWeightPipeline = vireo->createComputePipeline(smaaComputeResources, vireo->createShaderModule("shaders/smaa_blend_weight.comp"));
        smaaBlendPipeline       = vireo->createComputePipeline(smaaComputeResources, vireo->createShaderModule("shaders/smaa_neighborhood_blend.comp"));

        framesData.resize(framesInFlight);
        for (auto& frame : framesData) {
            frame.fxaaDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.fxaaDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.effectDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.effectDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.gammaCorrectionDescriptorSet = vireo->createDescriptorSet(descriptorLayout);
            frame.gammaCorrectionDescriptorSet->update(BINDING_PARAMS, paramsBuffer);
            frame.smaaComputeDescriptorSet = vireo->createDescriptorSet(smaaComputeDescLayout);
            frame.smaaComputeDescriptorSet->update(BINDING_PARAMS,    paramsBuffer);
            frame.smaaComputeDescriptorSet->update(SMAA_BINDING_DATA, smaaDataBuffer);
            frame.smaaExtraDescriptorSet = vireo->createDescriptorSet(smaaExtraDescLayout);
            frame.taaDescriptorSet[0] = vireo->createDescriptorSet(taaDescriptorLayout);
            frame.taaDescriptorSet[0]->update(BINDING_PARAMS, paramsBuffer);
            frame.taaDescriptorSet[1] = vireo->createDescriptorSet(taaDescriptorLayout);
            frame.taaDescriptorSet[1]->update(BINDING_PARAMS, paramsBuffer);
        }
    }

    void PostProcessing::onRender(
       const std::uint32_t frameIndex,
       const vireo::Extent& extent,
       const Samplers& samplers,
       const std::shared_ptr<vireo::CommandList>& cmdList,
       const std::shared_ptr<vireo::RenderTarget>& colorBuffer) {
        auto& frame = framesData[frameIndex];
        std::vector<std::shared_ptr<vireo::Image>> shaderReadTargets;

        if (!frame.colorBuffersInitialized) {
            cmdList->barrier(
                frame.effectColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->barrier(
                frame.gammaCorrectionColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->barrier(
                frame.fxaaColorBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            cmdList->barrier(
                frame.smaaEdgeBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::COMPUTE_READ);
            cmdList->barrier(
                frame.smaaBlendBuffer,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::COMPUTE_READ);
            cmdList->barrier(
                frame.smaaColorImage,
                vireo::ResourceState::UNDEFINED,
                vireo::ResourceState::RENDER_TARGET_COLOR);
        }

        if (applyEffect) {
            const auto colorInput =
                applyTAA ? frame.taaColorBuffer[taaIndex]->getImage() :
                colorBuffer->getImage();
            frame.effectDescriptorSet->update(BINDING_INPUT, colorInput);
            renderingConfig.colorRenderTargets[0].renderTarget = frame.effectColorBuffer;
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->beginRendering(renderingConfig);
            cmdList->bindPipeline(effectPipeline);
            cmdList->bindDescriptors({frame.effectDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            shaderReadTargets.push_back(colorInput);
        }

        if (applyGammaCorrection) {
            const auto colorInput =
                applyEffect ? frame.effectColorBuffer->getImage() :
                applyTAA ? frame.taaColorBuffer[taaIndex]->getImage() :
                colorBuffer->getImage();
            frame.gammaCorrectionDescriptorSet->update(BINDING_INPUT, colorInput);
            renderingConfig.colorRenderTargets[0].renderTarget = frame.gammaCorrectionColorBuffer;
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::RENDER_TARGET_COLOR,
                vireo::ResourceState::SHADER_READ);
            cmdList->beginRendering(renderingConfig);
            cmdList->bindPipeline(gammaCorrectionPipeline);
            cmdList->bindDescriptors({frame.gammaCorrectionDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            shaderReadTargets.push_back(colorInput);
        }

        if (applySMAA) {
            const auto colorInput =
                applyGammaCorrection ? frame.gammaCorrectionColorBuffer->getImage() :
                applyEffect ? frame.effectColorBuffer->getImage() :
                applyTAA ? frame.taaColorBuffer[taaIndex]->getImage() :
                colorBuffer->getImage();

            const std::vector<std::shared_ptr<vireo::Image>> smaaTextures = {
                colorInput,
                frame.smaaEdgeBuffer,
                frame.smaaBlendBuffer
            };
            frame.smaaComputeDescriptorSet->update(SMAA_BINDING_OUTPUT,   frame.smaaColorImage);
            frame.smaaComputeDescriptorSet->update(SMAA_BINDING_TEXTURES, smaaTextures);
            frame.smaaExtraDescriptorSet->update(SMAA_BINDING_EDGE,   frame.smaaEdgeBuffer);
            frame.smaaExtraDescriptorSet->update(SMAA_BINDING_WEIGHT, frame.smaaBlendBuffer);

            cmdList->bindDescriptors(
                vireo::PipelineType::COMPUTE,
                smaaComputeResources,
                { frame.smaaComputeDescriptorSet, samplers.getDescriptorSet(), frame.smaaExtraDescriptorSet });

            const auto tile_x = (extent.width  + TILE_SIZE - 1) / TILE_SIZE;
            const auto tile_y = (extent.height + TILE_SIZE - 1) / TILE_SIZE;

            cmdList->barrier(frame.smaaEdgeBuffer, vireo::ResourceState::COMPUTE_READ, vireo::ResourceState::COMPUTE_WRITE);
            cmdList->bindPipeline(smaaEdgePipeline);
            cmdList->dispatch(tile_x, tile_y, 1);
            cmdList->barrier(frame.smaaEdgeBuffer, vireo::ResourceState::COMPUTE_WRITE, vireo::ResourceState::COMPUTE_READ);

            cmdList->barrier(frame.smaaBlendBuffer, vireo::ResourceState::COMPUTE_READ, vireo::ResourceState::COMPUTE_WRITE);
            cmdList->bindPipeline(smaaBlendWeightPipeline);
            cmdList->dispatch(tile_x, tile_y, 1);
            cmdList->barrier(frame.smaaBlendBuffer, vireo::ResourceState::COMPUTE_WRITE, vireo::ResourceState::COMPUTE_READ);

            cmdList->barrier(frame.smaaColorImage, vireo::ResourceState::RENDER_TARGET_COLOR, vireo::ResourceState::COMPUTE_WRITE);
            cmdList->bindPipeline(smaaBlendPipeline);
            cmdList->dispatch(tile_x, tile_y, 1);
            cmdList->barrier(frame.smaaColorImage, vireo::ResourceState::COMPUTE_WRITE, vireo::ResourceState::RENDER_TARGET_COLOR);
        }

        if (applyFXAA) {
            const auto colorInput =
                applySMAA ? frame.smaaColorBuffer->getImage() :
                applyGammaCorrection ? frame.gammaCorrectionColorBuffer->getImage() :
                applyEffect ? frame.effectColorBuffer->getImage() :
                applyTAA ? frame.taaColorBuffer[taaIndex]->getImage() :
                colorBuffer->getImage();
            cmdList->barrier(
               colorInput,
               vireo::ResourceState::RENDER_TARGET_COLOR,
               vireo::ResourceState::SHADER_READ);
            frame.fxaaDescriptorSet->update(BINDING_INPUT, colorInput );
            renderingConfig.colorRenderTargets[0].renderTarget = frame.fxaaColorBuffer;
            cmdList->beginRendering(renderingConfig);
            cmdList->setViewport(vireo::Viewport{
                static_cast<float>(extent.width),
                static_cast<float>(extent.height)});
            cmdList->setScissors(vireo::Rect{
                extent.width,
                extent.height});
            cmdList->bindPipeline(fxaaPipeline);
            cmdList->bindDescriptors({frame.fxaaDescriptorSet, samplers.getDescriptorSet()});
            cmdList->draw(3);
            cmdList->endRendering();
            cmdList->barrier(
                colorInput,
                vireo::ResourceState::SHADER_READ,
                vireo::ResourceState::RENDER_TARGET_COLOR);
            shaderReadTargets.push_back(colorInput);
        }
        if (applyTAA) {
            taaIndex = (taaIndex + 1) % 2;
        }
        frame.colorBuffersInitialized = true;
    }

     std::shared_ptr<vireo::QueryPool>  PostProcessing::taaPass(
        const std::uint32_t frameIndex,
        const vireo::Extent& extent,
        const Samplers& samplers,
        const std::shared_ptr<vireo::CommandList>& cmdList,
        const std::shared_ptr<vireo::RenderTarget>& colorBuffer,
        const std::shared_ptr<vireo::RenderTarget>& velocityBuffer) {
        const auto& frame = framesData[frameIndex];
        if (!frame.colorBuffersInitialized) {
            for (const auto& buffer : frame.taaColorBuffer) {
                cmdList->barrier(
                    buffer,
                    vireo::ResourceState::UNDEFINED,
                    vireo::ResourceState::RENDER_TARGET_COLOR);
            }
        }

        if (!applyTAA) return nullptr;

        const auto historyIndex = (taaIndex + 1) % 2;
        const auto currentHistory = frame.taaColorBuffer[taaIndex];
        const auto previousHistory = frame.taaColorBuffer[historyIndex];

        cmdList->barrier(
           colorBuffer,
           vireo::ResourceState::RENDER_TARGET_COLOR,
           vireo::ResourceState::SHADER_READ);
        cmdList->barrier(
            previousHistory,
            vireo::ResourceState::RENDER_TARGET_COLOR,
            vireo::ResourceState::SHADER_READ);

        frame.taaDescriptorSet[taaIndex]->update(BINDING_INPUT, colorBuffer->getImage());
        frame.taaDescriptorSet[taaIndex]->update(BINDING_HISTORY, previousHistory->getImage());
        frame.taaDescriptorSet[taaIndex]->update(BINDING_VELOCITY, velocityBuffer->getImage());
        renderingConfig.colorRenderTargets[0].renderTarget = currentHistory;

        auto pool = vireo->createQueryPool(2, "TAA timing");
        cmdList->writeTimestamp(*pool, 0);

        cmdList->beginRendering(renderingConfig);
        cmdList->setViewport(vireo::Viewport{
            static_cast<float>(extent.width),
            static_cast<float>(extent.height)});
        cmdList->setScissors(vireo::Rect{
            extent.width,
            extent.height});
        cmdList->bindPipeline(taaPipeline);
        cmdList->bindDescriptors({frame.taaDescriptorSet[taaIndex], samplers.getDescriptorSet()});
        cmdList->draw(3);
        cmdList->endRendering();

        cmdList->barrier(
            previousHistory,
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);
        cmdList->barrier(
            colorBuffer,
            vireo::ResourceState::SHADER_READ,
            vireo::ResourceState::RENDER_TARGET_COLOR);

        cmdList->writeTimestamp(*pool, 1);
        cmdList->resolveQueryPool(*pool, 0, 2);
        return pool;
    }

    void PostProcessing::onResize(const vireo::Extent& extent) {
        params.imageSize.x = extent.width;
        params.imageSize.y = extent.height;
        taaIndex = 0;
        for (auto& frame : framesData) {
            frame.fxaaColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                vireo::RenderTargetType::COLOR, {},
                1, vireo::MSAA::NONE,
                "FXAA Color Buffer");
            frame.smaaColorImage = vireo->createReadWriteImage(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                1, 1,
                "SMAA Color Image");
            frame.smaaColorBuffer = vireo->createRenderTarget(frame.smaaColorImage);
            frame.smaaEdgeBuffer = vireo->createReadWriteImage(
                vireo::ImageFormat::R16G16_SFLOAT,
                extent.width, extent.height,
                1, 1,
                "SMAA Edge Buffer");
            frame.smaaBlendBuffer = vireo->createReadWriteImage(
                vireo::ImageFormat::R16G16_SFLOAT,
                extent.width, extent.height,
                1, 1,
                "SMAA Blend Buffer");
            frame.taaColorBuffer[0] = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                vireo::RenderTargetType::COLOR, {},
                1, vireo::MSAA::NONE,
                "TAA Color Buffer 0");
            frame.taaColorBuffer[1] = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                vireo::RenderTargetType::COLOR, {},
                1, vireo::MSAA::NONE,
                "TAA Color Buffer 1");
            frame.effectColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                vireo::RenderTargetType::COLOR, {},
                1, vireo::MSAA::NONE,
                "Effect Color Buffer");
            frame.gammaCorrectionColorBuffer = vireo->createRenderTarget(
                pipelineConfig.colorRenderFormats[0],
                extent.width, extent.height,
                vireo::RenderTargetType::COLOR, {},
                1, vireo::MSAA::NONE,
                "Gamma Correction Color Buffer");
        }
    }

    float PostProcessing::getCurrentTimeMilliseconds() {
        using namespace std::chrono;
        return static_cast<float>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }
}