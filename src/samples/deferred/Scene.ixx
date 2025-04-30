/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.deferred.scene;

import samples.deferred.global;

export namespace samples {

    class Scene {
    public:
        void onInit(
            const shared_ptr<vireo::Vireo>& vireo,
            const shared_ptr<vireo::CommandList>& uploadCommandList,
            float aspectRatio);

        void onUpdate();

        void onKeyDown(uint32_t key);

        void draw(const shared_ptr<vireo::CommandList>& cmdList) const;

        auto& getModel() const { return model; }

        auto& getGlobal() const { return global; }

        const auto& getMaterial() const { return material; }

        const auto& getTextures() const { return textures; }

    private:
        static constexpr auto TEXTURE_FORMAT = vireo::ImageFormat::R8G8B8A8_SRGB;

        Model model{};
        Global global{};
        Material material{};
        float cameraYRotationAngle{0.0f};
        vec3 cameraPos{0.0f, 0.0f, 2.0f};
        vec3 cameraTarget{0.0f, 0.0f, 0.0f};

        shared_ptr<vireo::Vireo>  vireo;
        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> indexBuffer;
        vector<shared_ptr<vireo::Image>>  textures;
        // shared_ptr<vireo::Image>  textureNormal;
        // shared_ptr<vireo::Image>  textureArm;

        // Models data
        std::vector<Vertex> cubeVertices = {
            // (+Z)
            { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { 0, 0 } },
            { {  0.5f, -0.5f,  0.5f }, { 0, 0, 1 }, { 1, 0 } },
            { {  0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { 1, 1 } },
            { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1 }, { 0, 1 } },

            // (-Z)
            { {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
            { { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 1, 0 } },
            { { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 1, 1 } },
            { {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1 } },

            // (+X)
            { { 0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 0 } },
            { { 0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 1, 0 } },
            { { 0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 1, 1 } },
            { { 0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 0, 1 } },

            // (-X)
            { { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0 } },
            { { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 1, 0 } },
            { { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 1, 1 } },
            { { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 0, 1 } },

            // (+Y)
            { { -0.5f, 0.5f,  0.5f }, { 0, 1, 0 }, { 0, 0 } },
            { {  0.5f, 0.5f,  0.5f }, { 0, 1, 0 }, { 1, 0 } },
            { {  0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 1, 1 } },
            { { -0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 0, 1 } },

            // (-Y)
            { { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 0, 0 } },
            { {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0 } },
            { {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 1 } },
            { { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 0, 1 } },
        };

        std::vector<uint32_t> cubeIndices = {
            0,  1,  2,  2,  3,  0, // back
            4,  5,  6,  6,  7,  4, // front
            8,  9,  10, 10, 11, 8, // right
            12, 13, 14, 14, 15, 12, // left
            16, 17, 18, 18, 19, 16, // top
            20, 21, 22, 22, 23, 20 // bottom
        };

        shared_ptr<vireo::Image> uploadTexture(
            const shared_ptr<vireo::CommandList>& uploadCommandList,
            const string& filename) const;

    };

}