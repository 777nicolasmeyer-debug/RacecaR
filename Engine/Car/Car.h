#ifndef RACECAR_CAR_H
#define RACECAR_CAR_H

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Camera/camera.h"
#include "../Math/Collisions.h"
#include "../Meshes/Meshes.h"
#include "../Scene/SceneManager.h"

class Car {
public:
    bool spawn(const std::vector<MeshData> &meshes, Camera &camera, Collisions &collisions);

    void despawn(Collisions &collisions);

    void drive(float throttle, bool braking, float steering, float deltaTime,
               const glm::vec3 &groundNormal, bool shiftUp, bool shiftDown);

    void updateTransform();

    void updateCamera(Camera &camera) const;

    bool isActive() const;

    float getRPM() const;

    int getGear() const;

    const glm::vec3 &getPosition() const;

    const glm::quat &getRotation() const;

    btRigidBody *getBody() const;

    std::vector<SceneObject> &getRenderObjects();

    const std::vector<SceneObject> &getRenderObjects() const;

private:
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    float rpm = 1500.0f;
    int gear = 1;
    btRigidBody *body = nullptr;
    SceneObject physicsObject;
    std::vector<SceneObject> renderObjects;
};

#endif
