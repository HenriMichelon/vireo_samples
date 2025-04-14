/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.hellotexture;

import samples.app;

export namespace samples {

    class TextureApp : public Application {
    public:
        void onInit() override;
        void onRender() override;
        void onDestroy() override;

    protected:

    private:
        struct Vertex {
            vec3 pos;
            vec2 uv;
        };

        const vireo::DescriptorIndex BINDING_TEXTURE{0};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::VertexInputLayout::R32G32B32_FLOAT, 0},
            {"TEXCOORD",    vireo::VertexInputLayout::R32G32_FLOAT, 12},
        };

        shared_ptr<vireo::Buffer> vertexBuffer;
        vector<shared_ptr<vireo::Image>> textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

        vector<shared_ptr<vireo::FrameData>> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandAllocator>> graphicCommandAllocator{vireo::SwapChain::FRAMES_IN_FLIGHT};
        vector<shared_ptr<vireo::CommandList>> graphicCommandList{vireo::SwapChain::FRAMES_IN_FLIGHT};

        shared_ptr<vireo::DescriptorLayout> descriptorLayout;
        shared_ptr<vireo::DescriptorLayout> samplersDescriptorLayout;

        map<string, shared_ptr<vireo::PipelineResources>> pipelineResources;
        map<string, shared_ptr<vireo::Pipeline>> pipelines;

        static constexpr auto TextureWidth = 256;
        static constexpr auto TextureHeight = 256;
        static constexpr auto TexturePixelSize = 4;
        vector<unsigned char> generateTextureData() const;
    };
}