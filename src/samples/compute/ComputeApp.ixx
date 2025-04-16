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
        void onRender() override;
        void onDestroy() override;

    private:
        const vireo::DescriptorIndex BINDING_PARAMS{0};
        const vireo::DescriptorIndex BINDING_IMAGE{1};

        struct Params {
            ivec2 imageSize{};
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
        shared_ptr<vireo::Buffer>           paramBuffer;
        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::Pipeline>         pipeline;
    };
}