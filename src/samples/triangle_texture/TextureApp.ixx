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

    private:
        const vireo::DescriptorIndex BINDING_TEXTURE{0};
        const vireo::DescriptorIndex BINDING_SAMPLERS{0};

        struct Vertex {
            vec3 pos;
            vec2 uv;
        };
        const vector<vireo::VertexInputLayout::AttributeDescription> vertexAttributes{
            {"POSITION", vireo::VertexInputLayout::R32G32B32_FLOAT, 0},
            {"TEXCOORD",    vireo::VertexInputLayout::R32G32_FLOAT, 12},
        };

        vector<Vertex>                     triangleVertices;
        shared_ptr<vireo::Buffer>          vertexBuffer;
        vector<shared_ptr<vireo::Image>>   textures;
        vector<shared_ptr<vireo::Sampler>> samplers;

        struct FrameData {
            shared_ptr<vireo::FrameData>        frameData;
            shared_ptr<vireo::CommandAllocator> commandAllocator;
            shared_ptr<vireo::CommandList>      commandList;
            shared_ptr<vireo::DescriptorSet>    descriptorSet;
            shared_ptr<vireo::DescriptorSet>    samplersDescriptorSet;
        };
        vector<FrameData> framesData{vireo::SwapChain::FRAMES_IN_FLIGHT};

        shared_ptr<vireo::DescriptorLayout>      descriptorLayout;
        shared_ptr<vireo::DescriptorLayout>      samplersDescriptorLayout;
        map<string, shared_ptr<vireo::Pipeline>> pipelines;

        static vector<unsigned char> generateTextureData(uint32_t width, uint32_t height);
    };
}