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
#include <string>
#include <vector>

struct SceneObject {
    VAO_VBO buffers;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::quat localAnimationRotation{1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<glm::mat4> jointMatrices;
    std::vector<int> skinJointNodes;
    std::vector<glm::mat4> inverseBindMatrices;
    glm::mat4 inverseMeshNodeTransform{1.0f};
    bool skinningEnabled = false;
    btRigidBody* body = nullptr;
    GLuint textureID = 0;
    std::string nodeName;
    glm::vec4 baseColor{1.0f};
    glm::vec3 localCenter{0.0f};
    bool transparent = false;
    bool alphaMasked = false;
    float alphaCutoff = 0.5f;

    glm::mat4 modelMatrix() const;
    void uploadSkinning(const Shader& shader) const;
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
