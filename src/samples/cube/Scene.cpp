/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
module;
#include <stb_image.h>
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
        this->vireo = vireo;
        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());

        material.diffuseTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, vireo::ImageFormat::R8G8B8A8_SRGB,
            "seaworn_stone_tiles_1k/seaworn_stone_tiles_diff_1k.png"));
        material.normalTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, vireo::ImageFormat::R8G8B8A8_UNORM,
            "seaworn_stone_tiles_1k/seaworn_stone_tiles_nor_gl_1k.png"));
        material.armTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, vireo::ImageFormat::R8G8B8A8_UNORM,
            "seaworn_stone_tiles_1k/seaworn_stone_tiles_arm_1k.png"));
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);

        global.cameraPosition = cameraPos;
        global.view = lookAt(cameraPos, cameraTarget, up);
        global.viewInverse = inverse(global.view);
        global.projection = perspective(radians(75.0f), aspectRatio, 0.1f, 100.0f);
        // model.transform = translate(model.transform, vec3(0.0f, -1.0f, 0.0f));
        constexpr  float angle = radians(-45.0f);
        model.transform = glm::rotate(model.transform, angle, AXIS_X);
        model.transform = glm::rotate(model.transform, angle, AXIS_Y);
    }

    void Scene::onUpdate() {
        // constexpr  float angle = radians(-0.1);
        // model.transform = glm::rotate(model.transform, angle, AXIS_X);
        // model.transform = glm::rotate(model.transform, angle, AXIS_Y);
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

    shared_ptr<vireo::Image> Scene::uploadTexture(
        const shared_ptr<vireo::CommandList>& uploadCommandList,
        const vireo::ImageFormat format,
        const string& filename) const {
        const auto pixelSize =vireo::Image::getPixelSize(format);
        int width, height, channels;
        stbi_uc* pixels = stbi_load(("res/" + filename).c_str(), &width, &height,&channels, pixelSize);
        if (!pixels) {
            throw runtime_error("Failed to load texture: " + filename);
        }
        const auto buffer = vireo->createBuffer(vireo::BufferType::TRANSFER, width * height * pixelSize);
        buffer->map();
        buffer->write(pixels);
        buffer->unmap();
        stbi_image_free(pixels);
        auto texture = vireo->createImage(format, width, height, 1, 1, to_wstring(filename));
        uploadCommandList->barrier(texture, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST);
        uploadCommandList->copy(buffer, texture);
        uploadCommandList->barrier(texture, vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ);
        return texture;
    }

}