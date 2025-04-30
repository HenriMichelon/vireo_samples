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
            "rusty_metal_grid_diff_1k.png"));
        material.normalTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, vireo::ImageFormat::R8G8B8A8_UNORM,
            "rusty_metal_grid_nor_gl_1k.png"));
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
        constexpr  float angle = radians(-0.1);
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
        auto buffer = vireo->createBuffer(vireo::BufferType::TRANSFER, width * height * pixelSize);
        buffer->map();
        buffer->write(pixels);
        stbi_image_free(pixels);

        const auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) - 1;
        auto texture = vireo->createImage(format, width, height, mipLevels, 1, to_wstring(filename));
        uploadCommandList->barrier(texture, vireo::ResourceState::UNDEFINED, vireo::ResourceState::COPY_DST, 0, mipLevels);
        uploadCommandList->copy(buffer, texture);

        auto currentWidth = width;
        auto currentHeight = height;
        auto previousData = static_cast<unsigned char*>(buffer->getMappedAddress());

        // Continue generating mip levels until reaching 4x4 resolution
        auto mipLevel = 1;
        while (currentWidth > 4 || currentHeight > 4) {
            const auto w = (currentWidth > 1) ? currentWidth / 2 : 1;
            const auto h = (currentHeight > 1) ? currentHeight / 2 : 1;
            const auto dataSize = w * h * pixelSize;
            const auto dataVector = make_shared<vector<uint8_t>>(static_cast<vector<uint8_t>::size_type>(dataSize));
            const auto data = dataVector->data();
            // Generate mip data by averaging 2x2 blocks from the previous level
            for (auto y = 0; y < h; ++y) {
                for (auto x = 0; x < w; ++x) {
                    const auto srcX = x * 2;
                    const auto srcY = y * 2;
                    // Average 2x2 block
                    for (int c = 0; c < pixelSize; ++c) {
                        const auto sum = previousData[(srcY * currentWidth + srcX) * pixelSize + c] +
                                       previousData[(srcY * currentWidth + (srcX + 1)) * pixelSize + c] +
                                       previousData[((srcY + 1) * currentWidth + srcX) * pixelSize + c] +
                                       previousData[((srcY + 1) * currentWidth + (srcX + 1)) * pixelSize + c];
                        data[(y * w + x) * pixelSize + c] = static_cast<uint8_t>(sum / pixelSize);
                    }
                }
            }
            buffer = vireo->createBuffer(vireo::BufferType::TRANSFER, w * h * pixelSize);
            buffer->map();
            buffer->write(data);
            uploadCommandList->copy(buffer, texture, 0, mipLevel);
            currentWidth = w;
            currentHeight = h;
            previousData = static_cast<unsigned char*>(buffer->getMappedAddress());
            mipLevel += 1;
        }
        uploadCommandList->barrier(texture, vireo::ResourceState::COPY_DST, vireo::ResourceState::SHADER_READ, 0, mipLevels);

        return texture;
    }

}