/* Copyright (c) 2025-present Henri Michelon
*
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
export module samples.common.samplers;

export namespace samples {

    class Samplers {
    public:
        static constexpr auto SAMPLER_NEAREST_BORDER{0};
        static constexpr auto SAMPLER_LINEAR_EDGE{1};

        void onInit(const std::shared_ptr<vireo::Vireo>& vireo);

        const auto& getDescriptorLayout() const { return descriptorLayout; }
        const auto& getDescriptorSet() const { return descriptorSet; }

    private:
        std::vector<std::shared_ptr<vireo::Sampler>> samplers;
        std::shared_ptr<vireo::DescriptorLayout>     descriptorLayout;
        std::shared_ptr<vireo::DescriptorSet>        descriptorSet;
    };

}