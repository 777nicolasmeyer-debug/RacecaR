//
// Created by 777ni on 2026/09/10.
//

#ifndef RACECAR_MESHES_H
#define RACECAR_MESHES_H
#include <glm/glm.hpp>
#include <vector>
#include <BulletDynamics/Dynamics/btRigidBody.h>


struct MeshData {
 std::vector<float> vertices;
 std::vector<unsigned int> indices;
 unsigned int textureID = 0;
 int indexType;
 glm::vec4 baseColor = glm::vec4(1.0f);
 glm::vec3 localCenter{0.0f};
 bool transparent = false;
 bool alphaMasked = false;
 float alphaCutoff = 0.5f;
};
#endif // RACECAR_MESHES_H
