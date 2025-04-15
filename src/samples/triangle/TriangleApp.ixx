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
        static constexpr float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};

        struct Vertex {
            vec3 pos;
            vec3 color;
        };
        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::AttributeFormat::R32G32B32_FLOAT, 0},
            {"COLOR",    vireo::AttributeFormat::R32G32B32_FLOAT, 12}
        };
        vector<Vertex>            triangleVertices{
                { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f} },
                { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
                { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        shared_ptr<vireo::Buffer> vertexBuffer;

        struct FrameData {
            shared_ptr<vireo::FrameData>        frameData;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        static constexpr auto pipelineConfig = vireo::Pipeline::Configuration {};
        map<string, shared_ptr<vireo::Pipeline>> pipelines;
    };
}