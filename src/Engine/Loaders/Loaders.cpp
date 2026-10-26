#include "Loaders.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {
    GLenum getImageFormat(const int components) {
        switch (components) {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default: return 0;
        }
    }

    void setTextureParameters() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    const unsigned char *getElementData(const tinygltf::Model &model,
                                        const tinygltf::Accessor &accessor,
                                        const size_t index) {
        if (index >= accessor.count || accessor.bufferView < 0 ||
            static_cast<size_t>(accessor.bufferView) >= model.bufferViews.size()) {
            return nullptr;
        }

        const auto &view = model.bufferViews[accessor.bufferView];
        if (view.buffer < 0 || static_cast<size_t>(view.buffer) >= model.buffers.size()) {
            return nullptr;
        }

        const int stride = accessor.ByteStride(view);
        if (stride < 0) {
            return nullptr;
        }
        const int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
        if (componentSize <= 0 || componentCount <= 0 ||
            stride < componentSize * componentCount) {
            return nullptr;
        }
        const auto &buffer = model.buffers[view.buffer].data;
        const size_t offset = view.byteOffset + accessor.byteOffset + index * static_cast<size_t>(stride);
        const size_t elementSize = static_cast<size_t>(componentSize * componentCount);
        const size_t viewEnd = view.byteOffset + view.byteLength;
        if (offset > buffer.size() || elementSize > buffer.size() - offset ||
            offset > viewEnd || elementSize > viewEnd - offset) {
            return nullptr;
        }
        return buffer.data() + offset;
    }

    float readAttributeComponent(const unsigned char *data, const int componentType,
                                 const bool normalized) {
        switch (componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                float value;
                std::memcpy(&value, data, sizeof(value));
                return value;
            }
            case TINYGLTF_COMPONENT_TYPE_BYTE: {
                int8_t value;
                std::memcpy(&value, data, sizeof(value));
                return normalized
                           ? std::max(static_cast<float>(value) / 127.0f, -1.0f)
                           : static_cast<float>(value);
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                uint8_t value;
                std::memcpy(&value, data, sizeof(value));
                return normalized
                           ? static_cast<float>(value) / 255.0f
                           : static_cast<float>(value);
            }
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                int16_t value;
                std::memcpy(&value, data, sizeof(value));
                return normalized
                           ? std::max(static_cast<float>(value) / 32767.0f, -1.0f)
                           : static_cast<float>(value);
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                uint16_t value;
                std::memcpy(&value, data, sizeof(value));
                return normalized
                           ? static_cast<float>(value) / 65535.0f
                           : static_cast<float>(value);
            }
            default:
                return 0.0f;
        }
    }

    bool isSupportedAttributeComponent(const int componentType) {
        return componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
               componentType == TINYGLTF_COMPONENT_TYPE_BYTE ||
               componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
               componentType == TINYGLTF_COMPONENT_TYPE_SHORT ||
               componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    }

    bool readIndex(const unsigned char *data, const int componentType, unsigned int &index) {
        switch (componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                uint8_t value;
                std::memcpy(&value, data, sizeof(value));
                index = value;
                return true;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                uint16_t value;
                std::memcpy(&value, data, sizeof(value));
                index = value;
                return true;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                std::memcpy(&index, data, sizeof(index));
                return true;
            default:
                return false;
        }
    }

    const tinygltf::Accessor *findAccessor(const tinygltf::Model &model,
                                           const tinygltf::Primitive &primitive,
                                           const std::string &attribute) {
        const auto found = primitive.attributes.find(attribute);
        if (found == primitive.attributes.end() || found->second < 0 ||
            static_cast<size_t>(found->second) >= model.accessors.size()) {
            return nullptr;
        }
        return &model.accessors[found->second];
    }
}

void Loaders::initialize() {
    if (whiteTexture == 0) {
        whiteTexture = createWhiteTexture();
    }
}

bool Loaders::loadModelFromFile(const char *path, tinygltf::Model &model) {
    tinygltf::TinyGLTF loader;
    std::string error;
    std::string warning;
    const std::string filePath(path);
    const bool isGltf = filePath.size() >= 5 &&
                        filePath.substr(filePath.size() - 5) == ".gltf";
    const bool loaded = isGltf
                            ? loader.LoadASCIIFromFile(&model, &error, &warning, path)
                            : loader.LoadBinaryFromFile(&model, &error, &warning, path);

    if (!warning.empty()) {
        std::cerr << "TinyGLTF warning: " << warning << std::endl;
    }
    if (!error.empty()) {
        std::cerr << "TinyGLTF error: " << error << std::endl;
    }
    if (!loaded) {
        std::cerr << "Failed to load model: " << path << std::endl;
        return false;
    }

    return true;
}

GLuint Loaders::createTextureFromImage(const tinygltf::Image &image) {
    const GLenum format = getImageFormat(image.component);
    if (format == 0 || image.width <= 0 || image.height <= 0 || image.image.empty()) {
        std::cerr << "Cannot create texture from an invalid glTF image" << std::endl;
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLint previousAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), image.width, image.height, 0,
                 format, GL_UNSIGNED_BYTE, image.image.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
    glGenerateMipmap(GL_TEXTURE_2D);
    setTextureParameters();
    return texture;
}

GLuint Loaders::createWhiteTexture() {
    constexpr unsigned char whitePixel[]{255, 255, 255, 255};
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}

ModelAnimationData Loaders::extractAnimationData(const tinygltf::Model &model) const {
    ModelAnimationData result;
    result.nodes.resize(model.nodes.size());
    for (size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
        const auto &source = model.nodes[nodeIndex];
        auto &node = result.nodes[nodeIndex];
        if (source.translation.size() == 3) {
            node.translation = glm::vec3(static_cast<float>(source.translation[0]),
                                         static_cast<float>(source.translation[1]),
                                         static_cast<float>(source.translation[2]));
        }
        if (source.rotation.size() == 4) {
            node.rotation = glm::quat(static_cast<float>(source.rotation[3]),
                                      static_cast<float>(source.rotation[0]),
                                      static_cast<float>(source.rotation[1]),
                                      static_cast<float>(source.rotation[2]));
        }
        if (source.scale.size() == 3) {
            node.scale = glm::vec3(static_cast<float>(source.scale[0]),
                                   static_cast<float>(source.scale[1]),
                                   static_cast<float>(source.scale[2]));
        }
        if (source.matrix.size() == 16) {
            node.usesMatrix = true;
            for (size_t column = 0; column < 4; ++column) {
                for (size_t row = 0; row < 4; ++row) {
                    node.matrix[column][row] =
                        static_cast<float>(source.matrix[column * 4 + row]);
                }
            }
        }
        for (const int child : source.children) {
            if (child >= 0 && static_cast<size_t>(child) < result.nodes.size()) {
                result.nodes[child].parent = static_cast<int>(nodeIndex);
            }
        }
    }

    for (const auto &sourceClip : model.animations) {
        AnimationClipData clip;
        clip.name = sourceClip.name;
        for (const auto &sourceChannel : sourceClip.channels) {
            if (sourceChannel.target_node < 0 ||
                static_cast<size_t>(sourceChannel.target_node) >= result.nodes.size() ||
                sourceChannel.sampler < 0 ||
                static_cast<size_t>(sourceChannel.sampler) >= sourceClip.samplers.size()) {
                continue;
            }
            AnimationPath path;
            size_t componentCount = 0;
            if (sourceChannel.target_path == "translation") {
                path = AnimationPath::Translation;
                componentCount = 3;
            } else if (sourceChannel.target_path == "rotation") {
                path = AnimationPath::Rotation;
                componentCount = 4;
            } else if (sourceChannel.target_path == "scale") {
                path = AnimationPath::Scale;
                componentCount = 3;
            } else {
                continue;
            }

            const auto &sampler = sourceClip.samplers[sourceChannel.sampler];
            if (sampler.interpolation == "CUBICSPLINE" ||
                sampler.input < 0 || sampler.output < 0 ||
                static_cast<size_t>(sampler.input) >= model.accessors.size() ||
                static_cast<size_t>(sampler.output) >= model.accessors.size()) {
                std::cerr << "Skipping unsupported glTF animation channel in clip "
                          << sourceClip.name << std::endl;
                continue;
            }
            const auto &input = model.accessors[sampler.input];
            const auto &output = model.accessors[sampler.output];
            if (input.type != TINYGLTF_TYPE_SCALAR ||
                input.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                output.type != (componentCount == 4 ? TINYGLTF_TYPE_VEC4 : TINYGLTF_TYPE_VEC3) ||
                output.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
                output.count != input.count) {
                std::cerr << "Skipping malformed glTF animation channel in clip "
                          << sourceClip.name << std::endl;
                continue;
            }

            AnimationChannelData channel;
            channel.node = sourceChannel.target_node;
            channel.path = path;
            channel.interpolation = sampler.interpolation;
            channel.times.reserve(input.count);
            channel.values.reserve(output.count * componentCount);
            bool valid = true;
            for (size_t key = 0; key < input.count; ++key) {
                const unsigned char *timeData = getElementData(model, input, key);
                const unsigned char *valueData = getElementData(model, output, key);
                if (timeData == nullptr || valueData == nullptr) {
                    valid = false;
                    break;
                }
                float time = 0.0f;
                std::memcpy(&time, timeData, sizeof(time));
                channel.times.push_back(time);
                clip.duration = std::max(clip.duration, time);
                for (size_t component = 0; component < componentCount; ++component) {
                    float value = 0.0f;
                    std::memcpy(&value, valueData + component * sizeof(float), sizeof(value));
                    channel.values.push_back(value);
                }
            }
            if (valid && !channel.times.empty()) {
                clip.channels.push_back(std::move(channel));
            }
        }
        if (!clip.channels.empty()) {
            result.clips.push_back(std::move(clip));
        }
    }
    return result;
}

MeshData Loaders::extractMeshData(const tinygltf::Model &model, const int meshIndex,
                                 const int skinIndex,
                                 const glm::mat4 &meshNodeTransform) {
    MeshData data;
    if (meshIndex < 0 || static_cast<size_t>(meshIndex) >= model.meshes.size()) {
        std::cerr << "Invalid glTF mesh index: " << meshIndex << std::endl;
        return data;
    }

    if (skinIndex >= 0 && static_cast<size_t>(skinIndex) < model.skins.size()) {
        const auto &skin = model.skins[skinIndex];
        if (!skin.joints.empty() && skin.joints.size() <= 64) {
            data.skinJointNodes = skin.joints;
            data.inverseMeshNodeTransform = glm::inverse(meshNodeTransform);
            data.inverseBindMatrices.assign(skin.joints.size(), glm::mat4(1.0f));
            if (skin.inverseBindMatrices >= 0 &&
                static_cast<size_t>(skin.inverseBindMatrices) < model.accessors.size()) {
                const auto &accessor = model.accessors[skin.inverseBindMatrices];
                if (accessor.type != TINYGLTF_TYPE_MAT4 ||
                    accessor.count < skin.joints.size() ||
                    accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    std::cerr << "Invalid inverse bind matrices for glTF skin" << std::endl;
                    data.skinJointNodes.clear();
                    data.inverseBindMatrices.clear();
                } else {
                    for (size_t joint = 0; joint < skin.joints.size(); ++joint) {
                        const unsigned char *element = getElementData(model, accessor, joint);
                        if (element == nullptr) {
                            data.skinJointNodes.clear();
                            data.inverseBindMatrices.clear();
                            break;
                        }
                        for (size_t column = 0; column < 4; ++column) {
                            for (size_t row = 0; row < 4; ++row) {
                                float value = 0.0f;
                                std::memcpy(&value, element + (column * 4 + row) * sizeof(float),
                                            sizeof(float));
                                data.inverseBindMatrices[joint][column][row] = value;
                            }
                        }
                    }
                }
            }
        } else if (skin.joints.size() > 64) {
            std::cerr << "Skipping glTF skin with more than 64 joints" << std::endl;
        }
    }

    const auto &mesh = model.meshes[meshIndex];
    for (const auto &primitive: mesh.primitives) {
        if (primitive.mode != -1 && primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            std::cerr << "Skipping non-triangle glTF primitive in mesh " << meshIndex << std::endl;
            continue;
        }
        const tinygltf::Accessor *positions = findAccessor(model, primitive, "POSITION");
        if (positions == nullptr || positions->type != TINYGLTF_TYPE_VEC3) {
            std::cerr << "Skipping glTF primitive without valid POSITION data" << std::endl;
            continue;
        }

        const tinygltf::Accessor *normals = findAccessor(model, primitive, "NORMAL");
        const tinygltf::Accessor *uvs = findAccessor(model, primitive, "TEXCOORD_0");
        const tinygltf::Accessor *joints = findAccessor(model, primitive, "JOINTS_0");
        const tinygltf::Accessor *weights = findAccessor(model, primitive, "WEIGHTS_0");
        const bool hasSkinAttributes =
            !data.skinJointNodes.empty() && joints != nullptr && weights != nullptr &&
            joints->type == TINYGLTF_TYPE_VEC4 && weights->type == TINYGLTF_TYPE_VEC4 &&
            (joints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
             joints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) &&
            isSupportedAttributeComponent(weights->componentType);
        const int jointComponentSize = joints == nullptr
                                           ? 0
                                           : tinygltf::GetComponentSizeInBytes(joints->componentType);
        const int weightComponentSize = weights == nullptr
                                            ? 0
                                            : tinygltf::GetComponentSizeInBytes(weights->componentType);
        if (hasSkinAttributes) {
            data.skinned = true;
        }
        const size_t baseVertexCount = data.vertices.size() / 8;
        const size_t baseIndexCount = data.indices.size();
        std::vector<unsigned int> indices;
        if (primitive.indices < 0) {
            indices.resize(positions->count);
            for (size_t i = 0; i < indices.size(); ++i) {
                indices[i] = static_cast<unsigned int>(i);
            }
        } else if (static_cast<size_t>(primitive.indices) < model.accessors.size()) {
            const auto &indexAccessor = model.accessors[primitive.indices];
            if (indexAccessor.type != TINYGLTF_TYPE_SCALAR) {
                std::cerr << "Skipping glTF primitive with non-scalar indices" << std::endl;
                continue;
            }
            indices.reserve(indexAccessor.count);
            bool validIndices = true;
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                const unsigned char *element = getElementData(model, indexAccessor, i);
                unsigned int index = 0;
                if (element == nullptr ||
                    !readIndex(element, indexAccessor.componentType, index)) {
                    validIndices = false;
                    break;
                }
                indices.push_back(index);
            }
            if (!validIndices) {
                std::cerr << "Skipping glTF primitive with invalid indices" << std::endl;
                continue;
            }
        } else {
            std::cerr << "Skipping glTF primitive with invalid index accessor" << std::endl;
            continue;
        }
        if (indices.size() % 3 != 0) {
            std::cerr << "Skipping glTF primitive with an incomplete triangle" << std::endl;
            continue;
        }

        const int positionComponentSize =
                tinygltf::GetComponentSizeInBytes(positions->componentType);
        const int positionComponentCount = tinygltf::GetNumComponentsInType(positions->type);
        const int normalComponentSize = normals == nullptr
                                            ? 0
                                            : tinygltf::GetComponentSizeInBytes(normals->componentType);
        const int uvComponentSize =
                uvs == nullptr ? 0 : tinygltf::GetComponentSizeInBytes(uvs->componentType);
        if (positionComponentSize <= 0 || positionComponentCount != 3 ||
            !isSupportedAttributeComponent(positions->componentType) ||
            (normals != nullptr && normals->type != TINYGLTF_TYPE_VEC3) ||
            (normals != nullptr &&
             (!isSupportedAttributeComponent(normals->componentType) || normalComponentSize <= 0)) ||
            (uvs != nullptr &&
             (uvs->type != TINYGLTF_TYPE_VEC2 ||
              !isSupportedAttributeComponent(uvs->componentType) || uvComponentSize <= 0))) {
            std::cerr << "Skipping glTF primitive with unsupported vertex attributes" << std::endl;
            continue;
        }

        const auto &materialIndex = primitive.material;
        if (materialIndex >= 0 && static_cast<size_t>(materialIndex) < model.materials.size()) {
            const auto &material = model.materials[materialIndex];
            const auto &color = material.pbrMetallicRoughness.baseColorFactor;
            data.baseColor = glm::vec4(static_cast<float>(color[0]),
                                       static_cast<float>(color[1]),
                                       static_cast<float>(color[2]),
                                       static_cast<float>(color[3]));
            data.transparent = material.alphaMode == "BLEND";
            data.alphaMasked = material.alphaMode == "MASK";
            data.alphaCutoff = static_cast<float>(material.alphaCutoff);

            const int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
            if (textureIndex >= 0 && static_cast<size_t>(textureIndex) < model.textures.size()) {
                const int imageIndex = model.textures[textureIndex].source;
                if (imageIndex >= 0 && static_cast<size_t>(imageIndex) < model.images.size()) {
                    data.textureID = createTextureFromImage(model.images[imageIndex]);
                }
            }
            if (data.textureID == 0) {
                data.textureID = whiteTexture;
            }
        } else {
            data.textureID = whiteTexture;
        }

        bool validPrimitive = true;
        for (const unsigned int index: indices) {
            if (index >= positions->count) {
                validPrimitive = false;
                break;
            }
            const unsigned char *positionData = getElementData(model, *positions, index);
            const unsigned char *normalData =
                    normals != nullptr && index < normals->count
                        ? getElementData(model, *normals, index)
                        : nullptr;
            const unsigned char *uvData =
                    uvs != nullptr && index < uvs->count ? getElementData(model, *uvs, index) : nullptr;
            const unsigned char *jointData =
                    hasSkinAttributes && index < joints->count ? getElementData(model, *joints, index) : nullptr;
            const unsigned char *weightData =
                    hasSkinAttributes && index < weights->count ? getElementData(model, *weights, index) : nullptr;
            if (positionData == nullptr) {
                validPrimitive = false;
                break;
            }

            for (int component = 0; component < 3; ++component) {
                data.vertices.push_back(readAttributeComponent(
                    positionData + component * positionComponentSize,
                    positions->componentType, positions->normalized));
            }
            for (int component = 0; component < 3; ++component) {
                data.vertices.push_back(
                    normalData == nullptr
                        ? 0.0f
                        : readAttributeComponent(normalData + component * normalComponentSize,
                                                 normals->componentType, normals->normalized));
            }
            for (int component = 0; component < 2; ++component) {
                data.vertices.push_back(
                    uvData == nullptr
                        ? 0.0f
                        : readAttributeComponent(uvData + component * uvComponentSize,
                                                 uvs->componentType, uvs->normalized));
            }
            glm::vec4 vertexJoints(0.0f);
            glm::vec4 vertexWeights(0.0f);
            if (jointData != nullptr && weightData != nullptr) {
                for (int component = 0; component < 4; ++component) {
                    vertexJoints[component] = readAttributeComponent(
                        jointData + component * jointComponentSize, joints->componentType, false);
                    vertexWeights[component] = readAttributeComponent(
                        weightData + component * weightComponentSize, weights->componentType,
                        weights->normalized);
                }
                const float totalWeight = vertexWeights.x + vertexWeights.y +
                                          vertexWeights.z + vertexWeights.w;
                if (totalWeight > 0.0f) {
                    vertexWeights /= totalWeight;
                }
            }
            for (int component = 0; component < 4; ++component) {
                data.skinAttributes.push_back(vertexJoints[component]);
            }
            for (int component = 0; component < 4; ++component) {
                data.skinAttributes.push_back(vertexWeights[component]);
            }
            data.indices.push_back(static_cast<unsigned int>(data.indices.size()));
        }

        if (!validPrimitive) {
            data.vertices.resize(baseVertexCount * 8);
            data.skinAttributes.resize(baseVertexCount * 8);
            data.indices.resize(baseIndexCount);
            std::cerr << "Skipping glTF primitive with invalid vertex data" << std::endl;
            continue;
        }
    }
    data.indexType = GL_UNSIGNED_INT;
    return data;
}
