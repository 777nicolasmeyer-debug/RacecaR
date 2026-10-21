//
// Created by 777ni on 2026/09/15.
//

#ifndef RACECAR_COLLISIONS_H
#define RACECAR_COLLISIONS_H
#include <glm/glm.hpp>
#include <btBulletCollisionCommon.h>
#include "../Meshes/Meshes.h"
#include"../Scene/SceneManager.h"
#include <iostream>

class Collisions {
public:
    void init();
    void update(float dt);
    void removeRigidBody(btRigidBody* body);
    glm::vec3 getGroundNormal(const btRigidBody* body) const;
    btRigidBody* convexShapeD(SceneObject& sceneObj, const MeshData& mesh,
                              const glm::vec3& scale = glm::vec3(1.0f));
    btRigidBody* boxShapeD(SceneObject& sceneObj, const MeshData& mesh,
                           const glm::vec3& scale = glm::vec3(1.0f));
    btRigidBody* triangleShapeS(SceneObject& sceneObj, const MeshData& mesh);
};


#endif // RACECAR_COLLISIONS_H
