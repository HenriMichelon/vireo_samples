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
module samples.common.scene;

namespace samples {

    void Scene::drawCube(const std::shared_ptr<vireo::CommandList>& cmdList) const {
        cmdList->bindVertexBuffer(vertexBuffer);
        cmdList->bindIndexBuffer(indexBuffer);
        cmdList->drawIndexed(cubeIndices.size());
    }

    void Scene::onInit(
        const std::shared_ptr<vireo::Vireo>& vireo,
        const std::shared_ptr<vireo::CommandList>& uploadCommandList,
        std::vector<std::shared_ptr<vireo::Buffer>>& stagingBuffers,
        const float aspectRatio) {
        this->vireo = vireo;

        vertexBuffer = vireo->createBuffer(vireo::BufferType::VERTEX,sizeof(Vertex),cubeVertices.size());
        indexBuffer = vireo->createBuffer(vireo::BufferType::INDEX,sizeof(uint32_t),cubeIndices.size());
        uploadCommandList->upload(vertexBuffer, &cubeVertices[0]);
        uploadCommandList->upload(indexBuffer, &cubeIndices[0]);

        models.resize(2);
        materials.resize(2);

        materials[MATERIAL_ROCKS].diffuseTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, stagingBuffers, vireo::ImageFormat::R8G8B8A8_SRGB,
            "gray_rocks_diff_1k.jpg"));
        materials[MATERIAL_ROCKS].normalTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, stagingBuffers, vireo::ImageFormat::R8G8B8A8_UNORM,
            "gray_rocks_nor_gl_1k.jpg"));
        materials[MATERIAL_ROCKS].aoTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, stagingBuffers, vireo::ImageFormat::R8_UNORM,
            "gray_rocks_ao_1k.jpg"));

        materials[MATERIAL_GRID].diffuseTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, stagingBuffers, vireo::ImageFormat::R8G8B8A8_SRGB,
            "Net004A_1K-JPG_Color.png"));
        materials[MATERIAL_GRID].normalTextureIndex = textures.size();
        textures.push_back(uploadTexture(uploadCommandList, stagingBuffers, vireo::ImageFormat::R8G8B8A8_UNORM,
            "Net004A_1K-JPG_NormalGL.jpg"));

        global.view = glm::lookAt(global.cameraPosition, cameraTarget, AXIS_UP);
        global.viewInverse = glm::inverse(global.view);
        global.projection = glm::perspective(glm::radians(75.0f), aspectRatio, 0.05f, 50.0f);

        static constexpr float angle = glm::radians(-45.0f);
        models[MODEL_OPAQUE].transform = glm::rotate(models[MODEL_OPAQUE].transform, angle, AXIS_X);
        models[MODEL_OPAQUE].transform = glm::rotate(models[MODEL_OPAQUE].transform, angle, AXIS_Y);
        models[MODEL_TRANSPARENT].transform =
                glm::rotate(glm::mat4{1.0f}, cubeYRotationAngle, AXIS_Y) *
                glm::translate(glm::mat4{1.0f}, radius_transparent) *
                glm::scale(glm::mat4{1.0f}, scale_transparent) *
                glm::rotate(models[MODEL_OPAQUE].transform, -angle, AXIS_X) *
                glm::rotate(models[MODEL_OPAQUE].transform, -angle, AXIS_X);
    }

    void Scene::onUpdate() {
        if (rotateCube) {
            models[MODEL_OPAQUE].transform = glm::rotate(models[MODEL_OPAQUE].transform, angle_opaque, AXIS_X);
            models[MODEL_OPAQUE].transform = glm::rotate(models[MODEL_OPAQUE].transform, angle_opaque, AXIS_Y);

            cubeYRotationAngle += angle_transparent;
            models[MODEL_TRANSPARENT].transform =
                glm::rotate(glm::mat4{1.0f}, cubeYRotationAngle, AXIS_Y) *
                glm::translate(glm::mat4{1.0f}, radius_transparent) *
                glm::scale(glm::mat4{1.0f}, scale_transparent) *
                glm::rotate(models[MODEL_OPAQUE].transform, -angle_opaque, AXIS_X) *
                glm::rotate(models[MODEL_OPAQUE].transform, -angle_opaque, AXIS_X);
        }
    }

    void Scene::onKeyDown(const KeyScanCodes keyCode) {
        glm::vec3 axis;
        auto angle = glm::radians(2.0f);
        // std::cout << "key: " << static_cast<int>(keyCode) << std::endl;
        switch (keyCode) {
        case KeyScanCodes::SPACE:
            rotateCube = !rotateCube;
            return;
        case KeyScanCodes::W:
            global.cameraPosition.z -= 0.1f;
            global.view = lookAt(global.cameraPosition, cameraTarget, AXIS_Y);
            return;
        case KeyScanCodes::S:
            global.cameraPosition.z += 0.1f;
            global.view = lookAt(global.cameraPosition, cameraTarget, AXIS_Y);
            return;
        case KeyScanCodes::A:
            global.cameraPosition.x -= 0.1f;
            global.view = lookAt(global.cameraPosition, cameraTarget, AXIS_Y);
            return;
        case KeyScanCodes::D:
            global.cameraPosition.x += 0.1f;
            global.view = lookAt(global.cameraPosition, cameraTarget, AXIS_Y);
            return;
        case KeyScanCodes::LEFT:
            axis = AXIS_Y;
            break;
        case KeyScanCodes::RIGHT:
            axis = AXIS_Y;
            angle *= -1.0f;
            break;
        case KeyScanCodes::UP:
            if (cameraYRotationAngle <= glm::radians(-60.f)) { return; }
            axis = AXIS_X;
            angle *= -1.0f;
            cameraYRotationAngle += angle;
            break;
        case KeyScanCodes::DOWN:
            if (cameraYRotationAngle >= glm::radians(60.0f)) { return; }
            axis = AXIS_X;
            cameraYRotationAngle += angle;
            break;
        default:
            return;
        }
        const auto viewDir = cameraTarget - global.cameraPosition;
        const glm::vec3 rotatedDir = rotate(glm::mat4{1.0f}, angle, axis) * glm::vec4(viewDir, 0.0f);
        cameraTarget = global.cameraPosition + rotatedDir;
        global.view = lookAt(global.cameraPosition, cameraTarget, AXIS_Y);
    }

    std::shared_ptr<vireo::Image> Scene::uploadTexture(
        const std::shared_ptr<vireo::CommandList>& uploadCommandList,
        std::vector<std::shared_ptr<vireo::Buffer>>& stagingBuffer,
        const vireo::ImageFormat format,
        const std::string& filename) const {
        const auto pixelSize = vireo::Image::getPixelSize(format);

        int width, height, channels;
        stbi_uc* pixels = stbi_load(("res/" + filename).c_str(), &width, &height,&channels, pixelSize);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture: " + filename);
        }

        auto buffer = vireo->createBuffer(vireo::BufferType::IMAGE_UPLOAD, width * pixelSize, height);
        buffer->map();
        buffer->write(pixels);
        stbi_image_free(pixels);

        const auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) - 1;
        auto texture = vireo->createImage(
            format,
            width, height,
            mipLevels, 1,
            filename);
        uploadCommandList->barrier(
            texture,
            vireo::ResourceState::UNDEFINED,
            vireo::ResourceState::COPY_DST,
            0, mipLevels);
        uploadCommandList->copy(buffer, texture, 0, 0, false);
        stagingBuffer.push_back(buffer);

        // generating mip levels until reaching 4x4 resolution
        auto currentWidth = width;
        auto currentHeight = height;
        auto previousData = static_cast<unsigned char*>(buffer->getMappedAddress());
        auto mipLevel = 1;
        while (currentWidth > 4 || currentHeight > 4) {
            const auto w = (currentWidth > 1) ? currentWidth / 2 : 1;
            const auto h = (currentHeight > 1) ? currentHeight / 2 : 1;
            const auto dataSize = w * h * pixelSize;
            const auto dataVector = std::make_shared<std::vector<uint8_t>>(static_cast<std::vector<uint8_t>::size_type>(dataSize));
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
            buffer = vireo->createBuffer(vireo::BufferType::IMAGE_UPLOAD, w * pixelSize, h);
            buffer->map();
            buffer->write(data);
            uploadCommandList->copy(buffer, texture, 0, mipLevel, false);
            stagingBuffer.push_back(buffer);
            currentWidth = w;
            currentHeight = h;
            previousData = static_cast<unsigned char*>(buffer->getMappedAddress());
            mipLevel += 1;
        }
        uploadCommandList->barrier(
            texture,
            vireo::ResourceState::COPY_DST,
            vireo::ResourceState::SHADER_READ,
            0, mipLevels);
        return texture;
    }

}