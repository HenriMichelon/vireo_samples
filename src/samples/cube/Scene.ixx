/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.cube.scene;

import samples.cube.global;

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

        const auto& getLight() const { return light; }

        const auto& getTextures() const { return textures; }

    private:
        Model model{};
        Global global{};
        Material material{};
        Light light{};
        bool rotateCube{true};
        float cameraYRotationAngle{0.0f};
        vec3 cameraTarget{0.0f, 0.0f, 0.0f};

        shared_ptr<vireo::Vireo>  vireo;
        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> indexBuffer;
        vector<shared_ptr<vireo::Image>>  textures;

        // Models data
        std::vector<Vertex> cubeVertices = {
            // Face avant (-Z)
            {{-0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {1, 0}, {1, 0, 0}},
            {{ 0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {1, 1}, {1, 0, 0}},
            {{-0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {0, 1}, {1, 0, 0}},

            // Face arrière (+Z)
            {{ 0.5f, -0.5f, 0.5f}, { 0,  0, 1}, {0, 0}, {-1, 0, 0}},
            {{-0.5f, -0.5f, 0.5f}, { 0,  0, 1}, {1, 0}, {-1, 0, 0}},
            {{-0.5f,  0.5f, 0.5f}, { 0,  0, 1}, {1, 1}, {-1, 0, 0}},
            {{ 0.5f,  0.5f, 0.5f}, { 0,  0, 1}, {0, 1}, {-1, 0, 0}},

            // Face gauche (-X)
            {{-0.5f, -0.5f,  0.5f}, {-1,  0,  0}, {0, 0}, {0, 0, -1}},
            {{-0.5f, -0.5f, -0.5f}, {-1,  0,  0}, {1, 0}, {0, 0, -1}},
            {{-0.5f,  0.5f, -0.5f}, {-1,  0,  0}, {1, 1}, {0, 0, -1}},
            {{-0.5f,  0.5f,  0.5f}, {-1,  0,  0}, {0, 1}, {0, 0, -1}},

            // Face droite (+X)
            {{0.5f, -0.5f, -0.5f}, {1, 0, 0}, {0, 0}, {0, 0, 1}},
            {{0.5f, -0.5f,  0.5f}, {1, 0, 0}, {1, 0}, {0, 0, 1}},
            {{0.5f,  0.5f,  0.5f}, {1, 0, 0}, {1, 1}, {0, 0, 1}},
            {{0.5f,  0.5f, -0.5f}, {1, 0, 0}, {0, 1}, {0, 0, 1}},

            // Face bas (-Y)
            {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}, {1, 0, 0}},
            {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}, {1, 0, 0}},

            // Face haut (+Y)
            {{-0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, 0.5f, -0.5f}, {0, 1, 0}, {1, 0}, {1, 0, 0}},
            {{ 0.5f, 0.5f,  0.5f}, {0, 1, 0}, {1, 1}, {1, 0, 0}},
            {{-0.5f, 0.5f,  0.5f}, {0, 1, 0}, {0, 1}, {1, 0, 0}},
        };

        // CCW
        std::vector<uint32_t> cubeIndices = {
            0,  2,  1,   0,  3,  2,       // Avant (-Z)
            4,  6,  5,   4,  7,  6,       // Arrière (+Z)
            8, 10,  9,   8, 11, 10,       // Gauche (-X)
            12,14,13,   12,15,14,         // Droite (+X)
            16,18,17,   16,19,18,         // Bas (-Y)
            20,22,21,   20,23,22          // Haut (+Y)
        };



        shared_ptr<vireo::Image> uploadTexture(
          const shared_ptr<vireo::CommandList>& uploadCommandList,
          vireo::ImageFormat format,
          const string& filename) const;
    };

}