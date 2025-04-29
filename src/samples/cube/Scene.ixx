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

    private:
        Model model{};
        Global global{};
        float cameraYRotationAngle{0.0f};
        vec3 cameraPos{0.0f, 0.0f, 2.0f};
        vec3 cameraTarget{0.0f, 0.0f, 0.0f};

        shared_ptr<vireo::Buffer> vertexBuffer;
        shared_ptr<vireo::Buffer> indexBuffer;

        // Models data
        vector<Vertex> cubeVertices = {
            { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },

            { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
            { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f } },
            { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
        };

        vector<uint32_t> cubeIndices = {
            // front
            0, 1, 2,  2, 3, 0,
            // right
            1, 5, 6,  6, 2, 1,
            // back
            5, 4, 7,  7, 6, 5,
            // left
            4, 0, 3,  3, 7, 4,
            // top
            3, 2, 6,  6, 7, 3,
            // bottom
            4, 5, 1,  1, 0, 4
        };
    };

}