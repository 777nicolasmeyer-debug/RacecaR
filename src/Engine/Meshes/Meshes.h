//
// Created by 777ni on 2026/09/10.
//

#ifndef RACECAR_MESHES_H
#define RACECAR_MESHES_H
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <BulletDynamics/Dynamics/btRigidBody.h>


struct MeshData {
 std::vector<float> vertices;
 std::vector<float> skinAttributes;
 std::vector<unsigned int> indices;
 std::string nodeName;
 std::vector<int> skinJointNodes;
 std::vector<glm::mat4> inverseBindMatrices;
 glm::mat4 inverseMeshNodeTransform{1.0f};
 bool skinned = false;
 unsigned int textureID = 0;
 int indexType;
 glm::vec4 baseColor = glm::vec4(1.0f);
 glm::vec3 localCenter{0.0f};
 bool transparent = false;
 bool alphaMasked = false;
 float alphaCutoff = 0.5f;
};

enum class AnimationPath {
    Translation,
    Rotation,
    Scale
};

struct AnimationNodeData {
    int parent = -1;
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::mat4 matrix{1.0f};
    bool usesMatrix = false;
};

struct AnimationChannelData {
    int node = -1;
    AnimationPath path = AnimationPath::Translation;
    std::string interpolation;
    std::vector<float> times;
    std::vector<float> values;
};

struct AnimationClipData {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannelData> channels;
};

struct ModelAnimationData {
    std::vector<AnimationNodeData> nodes;
    std::vector<AnimationClipData> clips;
};
#endif // RACECAR_MESHES_H
