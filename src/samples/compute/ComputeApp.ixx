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

    private:
        const vireo::DescriptorIndex BINDING_PARAMS{0};
        const vireo::DescriptorIndex BINDING_IMAGE{1};
        static constexpr float MAX_FLOAT = numeric_limits<float>::max();

        struct Params {
            ivec2 imageSize{};
            float time;
        };

        struct FrameData {
            shared_ptr<vireo::FrameData>        frameData;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::Image>            image;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        Params                              params{};
        shared_ptr<vireo::Buffer>           paramsBuffer;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::Pipeline>         pipeline;

        static float getCurrentTimeMilliseconds();
    };
}