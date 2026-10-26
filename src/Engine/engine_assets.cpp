#include "Engine.h"

#include <functional>
#include <iostream>
#include <limits>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
    glm::mat4 getNodeTransform(const tinygltf::Node &node) {
        if (node.matrix.size() == 16) {
            glm::mat4 transform(1.0f);
            for (size_t column = 0; column < 4; ++column) {
                for (size_t row = 0; row < 4; ++row) {
                    transform[column][row] = static_cast<float>(node.matrix[column * 4 + row]);
                }
            }
            return transform;
        }

        glm::mat4 transform(1.0f);
        if (node.translation.size() == 3) {
            transform = glm::translate(transform, glm::vec3(
                                           static_cast<float>(node.translation[0]),
                                           static_cast<float>(node.translation[1]),
                                           static_cast<float>(node.translation[2])));
        }
        if (node.rotation.size() == 4) {
            transform *= glm::mat4_cast(glm::quat(
                static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2])));
        }
        if (node.scale.size() == 3) {
            transform = glm::scale(transform, glm::vec3(
                                       static_cast<float>(node.scale[0]),
                                       static_cast<float>(node.scale[1]),
                                       static_cast<float>(node.scale[2])));
        }
        return transform;
    }

    void applyNodeTransform(MeshData &mesh, const glm::mat4 &transform) {
        const glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(transform)));
        for (size_t i = 0; i + 7 < mesh.vertices.size(); i += 8) {
            const glm::vec3 position = glm::vec3(transform * glm::vec4(
                                                     mesh.vertices[i], mesh.vertices[i + 1], mesh.vertices[i + 2],
                                                     1.0f));
            mesh.vertices[i] = position.x;
            mesh.vertices[i + 1] = position.y;
            mesh.vertices[i + 2] = position.z;

            const glm::vec3 normal = normalTransform *
                                     glm::vec3(mesh.vertices[i + 3], mesh.vertices[i + 4], mesh.vertices[i + 5]);
            if (glm::dot(normal, normal) > 0.0f) {
                const glm::vec3 normalized = glm::normalize(normal);
                mesh.vertices[i + 3] = normalized.x;
                mesh.vertices[i + 4] = normalized.y;
                mesh.vertices[i + 5] = normalized.z;
            }
        }
    }
}

const std::vector<MeshData> &Engine::loadModelMeshes(const char *path) {
    const auto [entry, inserted] = modelCache.try_emplace(path);
    if (!inserted) {
        return entry->second;
    }

    tinygltf::Model model;
    if (!loader.loadModelFromFile(path, model)) {
        return entry->second;
    }
    animationCache[path] = loader.extractAnimationData(model);
    if (model.meshes.empty()) {
        std::cerr << "Model contains no meshes: " << path << std::endl;
        return entry->second;
    }

    std::vector<bool> hasParent(model.nodes.size(), false);
    for (const auto &node: model.nodes) {
        for (const int child: node.children) {
            if (child >= 0 && static_cast<size_t>(child) < hasParent.size()) {
                hasParent[child] = true;
            }
        }
    }

    std::function<void(int, const glm::mat4 &)> visitNode;
    visitNode = [&](const int nodeIndex, const glm::mat4 &parentTransform) {
        if (nodeIndex < 0 || static_cast<size_t>(nodeIndex) >= model.nodes.size()) {
            return;
        }
        const auto &node = model.nodes[nodeIndex];
        const glm::mat4 worldTransform = parentTransform * getNodeTransform(node);
        if (node.mesh >= 0 && static_cast<size_t>(node.mesh) < model.meshes.size()) {
            MeshData mesh = loader.extractMeshData(model, node.mesh, node.skin, worldTransform);
            mesh.nodeName = node.name;
            applyNodeTransform(mesh, worldTransform);
            if (!mesh.vertices.empty()) {
                glm::vec3 minimum(std::numeric_limits<float>::max());
                glm::vec3 maximum(std::numeric_limits<float>::lowest());
                for (size_t i = 0; i + 2 < mesh.vertices.size(); i += 8) {
                    const glm::vec3 point(mesh.vertices[i], mesh.vertices[i + 1],
                                          mesh.vertices[i + 2]);
                    minimum = glm::min(minimum, point);
                    maximum = glm::max(maximum, point);
                }
                mesh.localCenter = (minimum + maximum) * 0.5f;
                entry->second.push_back(std::move(mesh));
            }
        }
        for (const int child: node.children) {
            visitNode(child, worldTransform);
        }
    };

    if (!model.scenes.empty()) {
        const int sceneIndex = model.defaultScene >= 0 &&
                               static_cast<size_t>(model.defaultScene) < model.scenes.size()
                                   ? model.defaultScene
                                   : 0;
        for (const int rootNode: model.scenes[sceneIndex].nodes) {
            visitNode(rootNode, glm::mat4(1.0f));
        }
    } else {
        for (size_t i = 0; i < model.nodes.size(); ++i) {
            if (!hasParent[i]) {
                visitNode(static_cast<int>(i), glm::mat4(1.0f));
            }
        }
    }

    if (entry->second.empty()) {
        std::cerr << "Model scene contains no renderable meshes: " << path << std::endl;
    }
    return entry->second;
}
