/* Copyright (c) 2025-present Henri Michelon
*
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
module samples.common.samplers;

namespace samples {

    void Samplers::onInit(const std::shared_ptr<vireo::Vireo>& vireo) {
        samplers.push_back(vireo->createSampler(
          vireo::Filter::NEAREST,
          vireo::Filter::NEAREST,
          vireo::AddressMode::CLAMP_TO_BORDER,
          vireo::AddressMode::CLAMP_TO_BORDER,
          vireo::AddressMode::CLAMP_TO_BORDER));
        samplers.push_back(vireo->createSampler(
          vireo::Filter::LINEAR,
          vireo::Filter::LINEAR,
          vireo::AddressMode::CLAMP_TO_EDGE,
          vireo::AddressMode::CLAMP_TO_EDGE,
          vireo::AddressMode::CLAMP_TO_EDGE));

        descriptorLayout = vireo->createSamplerDescriptorLayout("Samplers");
        for (int i = 0; i < samplers.size(); i++) {
            descriptorLayout->add(i, vireo::DescriptorType::SAMPLER);
        }
        descriptorLayout->build();

        descriptorSet = vireo->createDescriptorSet(descriptorLayout, "Samplers");
        for (int i = 0; i < samplers.size(); i++) {
            descriptorSet->update(i, samplers[i]);
        }
    }

}