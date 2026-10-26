//
// Created by 777ni on 2026/09/11.
//

#ifndef RACECAR_LOADERS_H
#define RACECAR_LOADERS_H
#include <glad/glad.h>
#include "../Meshes/Meshes.h"
#include <tiny_gltf.h>

class Loaders {
public:
    void initialize();

    bool loadModelFromFile(const char *path, tinygltf::Model &model);

    GLuint createTextureFromImage(const tinygltf::Image &image);

    ModelAnimationData extractAnimationData(const tinygltf::Model &model) const;

    MeshData extractMeshData(const tinygltf::Model &model, int meshIndex,
                             int skinIndex = -1,
                             const glm::mat4 &meshNodeTransform = glm::mat4(1.0f));

private:
    GLuint createWhiteTexture();

    GLuint whiteTexture = 0;
};

#endif // RACECAR_LOADERS_H
