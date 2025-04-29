/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include <glm/gtc/matrix_transform.hpp>
#include "Libraries.h"
module samples.cube.scene;

namespace samples {

    void Scene::draw(const shared_ptr<vireo::CommandList>& cmdList) const {
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindIndexBuffer(indexBuffer);
        cmdList->drawIndexed(cubeIndices.size());
    }

    void Scene::onInit(
        const shared_ptr<vireo::Vireo>& vireo,
        const shared_ptr<vireo::CommandList>& uploadCommandList,
        const float aspectRatio) {
        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());

        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);

        global.view = lookAt(cameraPos, cameraTarget, up);
        global.projection = perspective(radians(75.0f), aspectRatio, 0.1f, 100.0f);
    }

    void Scene::onUpdate() {
        constexpr  float angle = radians(-1.0);
        model.transform = glm::rotate(model.transform, angle, AXIS_X);
        model.transform = glm::rotate(model.transform, angle, AXIS_Y);
    }

    void Scene::onKeyDown(const uint32_t key) {
        const auto keyCode = static_cast<KeyCodes>(key);
        vec3 axis;
        auto angle = radians(2.0f);
        switch (keyCode) {
        case KeyCodes::LEFT:
            axis = AXIS_Y;
            break;
        case KeyCodes::RIGHT:
            axis = AXIS_Y;
            angle *= -1.0f;
            break;
        case KeyCodes::UP:
            if (cameraYRotationAngle <= radians(-60.f)) { return; }
            axis = AXIS_X;
            angle *= -1.0f;
            cameraYRotationAngle += angle;
            break;
        case KeyCodes::DOWN:
            if (cameraYRotationAngle >= radians(60.0f)) { return; }
            axis = AXIS_X;
            cameraYRotationAngle += angle;
            break;
        default:
            return;
        }
        const auto viewDir = cameraTarget - cameraPos;
        const vec3 rotatedDir = rotate(mat4{1.0f}, angle, axis) * vec4(viewDir, 0.0f);
        cameraTarget = cameraPos + rotatedDir;

        global.view = lookAt(cameraPos, cameraTarget, AXIS_Y);
    }

}