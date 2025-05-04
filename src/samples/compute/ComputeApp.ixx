/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.compute;

import samples.app;

export namespace samples {

    class ComputeApp : public Application {
    public:
        void onInit() override;
        void onUpdate() override;
        void onRender() override;
        void onDestroy() override;
        void onResize() override;

    private:
        static constexpr float MAX_FLOAT = 3.4028235e+38;

        const vireo::DescriptorIndex BINDING_PARAMS{0};
        const vireo::DescriptorIndex BINDING_IMAGE{1};

        struct Params {
            glm::ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            std::shared_ptr<vireo::CommandAllocator> commandAllocator;
            std::shared_ptr<vireo::CommandList>      commandList;
            std::shared_ptr<vireo::DescriptorSet>    descriptorSet;
            std::shared_ptr<vireo::Fence>            inFlightFence;
            std::shared_ptr<vireo::Image>            image;
        };

        Params                                   params{};
        std::vector<FrameData>                   framesData;
        std::shared_ptr<vireo::Buffer>           paramsBuffer;
        std::shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        std::shared_ptr<vireo::Pipeline>         pipeline;
        std::shared_ptr<vireo::SwapChain>        swapChain;
        std::shared_ptr<vireo::SubmitQueue>      graphicSubmitQueue;

        static float getCurrentTimeMilliseconds();
    };
}