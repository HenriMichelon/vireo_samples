/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellotriangle;

import samples.app;

export namespace samples {

    class TriangleApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onDestroy() override;

    private:
        struct Vertex {
            vec3 pos;
            vec3 color;
        };
        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::VertexInputLayout::R32G32B32_FLOAT, 0},
            {"COLOR",    vireo::VertexInputLayout::R32G32B32_FLOAT, 12}
        };
        shared_ptr<vireo::Buffer> vertexBuffer;

        vector<shared_ptr<vireo::FrameData>> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandAllocator>> graphicCommandAllocator{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandList>> graphicCommandList{vireo::SwapChain::FRAMES_IN_FLIGHT};

        map<string, shared_ptr<vireo::PipelineResources>> pipelineResources;
        map<string, shared_ptr<vireo::Pipeline>> pipelines;
    };
}