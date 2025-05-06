/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include "Libraries.h"
export module samples.common.scene;

import samples.common.global;

export namespace samples {

    class Scene {
    public:
        static constexpr auto MODEL_OPAQUE{0};
        static constexpr auto MODEL_TRANSPARENT{1};
        static constexpr auto MATERIAL_ROCKS{0};
        static constexpr auto MATERIAL_GRID{1};

        void onInit(
            const std::shared_ptr<vireo::Vireo>& vireo,
            const std::shared_ptr<vireo::CommandList>& uploadCommandList,
            float aspectRatio);
        void onUpdate();
        void onKeyDown(KeyScanCodes keyCode);

        void drawCube(const std::shared_ptr<vireo::CommandList>& cmdList) const;

        const auto& getGlobal() const { return global; }
        const auto& getModels() const { return models; }
        const auto& getMaterials() const { return materials; }
        const auto& getLight() const { return light; }
        const auto& getTextures() const { return textures; }

    private:
        static constexpr float angle_opaque = glm::radians(-0.1);
        static constexpr float angle_transparent = glm::radians(-0.5);
        static constexpr auto scale_transparent = glm::vec3{0.25f, 0.25f, 0.25f};
        static constexpr auto radius_transparent = glm::vec3{1.25f, 0.0f, 0.0f};


        Global     global{};
        Light      light{};
        bool       rotateCube{false};
        float      cubeYRotationAngle{glm::radians(-45.0f)};
        float      cameraYRotationAngle{0.0f};
        glm::vec3  cameraTarget{0.0f, 0.0f, 0.0f};

        std::vector<Model>                         models;
        std::vector<Material>                      materials;
        std::shared_ptr<vireo::Vireo>              vireo;
        std::shared_ptr<vireo::Buffer>             vertexBuffer;
        std::shared_ptr<vireo::Buffer>             indexBuffer;
        std::vector<std::shared_ptr<vireo::Image>> textures;

        std::vector<Vertex> cubeVertices {
            // (-Z)
            {{-0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {1, 0}, {1, 0, 0}},
            {{ 0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {1, 1}, {1, 0, 0}},
            {{-0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {0, 1}, {1, 0, 0}},

            // (+Z)
            {{ 0.5f, -0.5f, 0.5f}, { 0,  0, 1}, {0, 0}, {-1, 0, 0}},
            {{-0.5f, -0.5f, 0.5f}, { 0,  0, 1}, {1, 0}, {-1, 0, 0}},
            {{-0.5f,  0.5f, 0.5f}, { 0,  0, 1}, {1, 1}, {-1, 0, 0}},
            {{ 0.5f,  0.5f, 0.5f}, { 0,  0, 1}, {0, 1}, {-1, 0, 0}},

            // (-X)
            {{-0.5f, -0.5f,  0.5f}, {-1,  0,  0}, {0, 0}, {0, 0, -1}},
            {{-0.5f, -0.5f, -0.5f}, {-1,  0,  0}, {1, 0}, {0, 0, -1}},
            {{-0.5f,  0.5f, -0.5f}, {-1,  0,  0}, {1, 1}, {0, 0, -1}},
            {{-0.5f,  0.5f,  0.5f}, {-1,  0,  0}, {0, 1}, {0, 0, -1}},

            // (+X)
            {{0.5f, -0.5f, -0.5f}, {1, 0, 0}, {0, 0}, {0, 0, 1}},
            {{0.5f, -0.5f,  0.5f}, {1, 0, 0}, {1, 0}, {0, 0, 1}},
            {{0.5f,  0.5f,  0.5f}, {1, 0, 0}, {1, 1}, {0, 0, 1}},
            {{0.5f,  0.5f, -0.5f}, {1, 0, 0}, {0, 1}, {0, 0, 1}},

            // (-Y)
            {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}, {1, 0, 0}},
            {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}, {1, 0, 0}},

            // (+Y)
            {{-0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0, 0}, {1, 0, 0}},
            {{ 0.5f, 0.5f, -0.5f}, {0, 1, 0}, {1, 0}, {1, 0, 0}},
            {{ 0.5f, 0.5f,  0.5f}, {0, 1, 0}, {1, 1}, {1, 0, 0}},
            {{-0.5f, 0.5f,  0.5f}, {0, 1, 0}, {0, 1}, {1, 0, 0}},
        };

        // CCW
        std::vector<uint32_t> cubeIndices {
            0,  2,  1,   0,  3,  2, // (-Z)
            4,  6,  5,   4,  7,  6, // (+Z)
            8, 10,  9,   8, 11, 10, // (-X)
            12,14,13,   12,15,14,   // (+X)
            16,18,17,   16,19,18,   // (-Y)
            20,22,21,   20,23,22    // (+Y)
        };

        std::shared_ptr<vireo::Image> uploadTexture(
            const std::shared_ptr<vireo::CommandList>& uploadCommandList,
            vireo::ImageFormat format,
            const std::string& filename) const;
    };

}