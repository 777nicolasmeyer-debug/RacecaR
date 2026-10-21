//
// Created by 777ni on 2026/09/16.
//

#ifndef RACECAR_SCENEMANAGER_H
#define RACECAR_SCENEMANAGER_H


#include "../Rendering/VAO_VBO.h"
#include "../Shader/Shader.h"
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

struct SceneObject {
    VAO_VBO buffers;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    btRigidBody* body = nullptr;
    GLuint textureID = 0;
    glm::vec4 baseColor{1.0f};
    glm::vec3 localCenter{0.0f};
    bool transparent = false;
    bool alphaMasked = false;
    float alphaCutoff = 0.5f;

    void draw(Shader& shader);
};

struct LightingObject {
    std::vector<VAO_VBO> buffers;
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    GLuint shadowCubemap{0};
    btRigidBody* body = nullptr;
    bool persistent = false;

    void draw(Shader& shader);
};


#endif // RACECAR_SCENEMANAGER_H
