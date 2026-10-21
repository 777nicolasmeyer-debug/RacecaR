//
// Created by 777ni on 2026/09/11.
//

#ifndef RACECAR_LOADERS_H
#define RACECAR_LOADERS_H
#include <glad/glad.h>
#include "../Meshes/Meshes.h"
#include <tiny_gltf.h>
#include <SFML/Audio.hpp>

class Loaders {
public:
    void initialize();

    bool loadModelFromFile(const char *path, tinygltf::Model &model);

    GLuint createTextureFromImage(const tinygltf::Image &image);

    MeshData extractMeshData(const tinygltf::Model &model, int meshIndex);

private:
    GLuint createWhiteTexture();

    GLuint whiteTexture = 0;
};

class Sound {
public:
    sf::SoundBuffer buffer;
    sf::Sound sound;
};

class Music {
public:
    sf::Music music;
};

#endif // RACECAR_LOADERS_H
