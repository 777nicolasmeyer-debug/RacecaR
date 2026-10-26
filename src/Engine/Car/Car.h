#ifndef RACECAR_CAR_H
#define RACECAR_CAR_H

#include <array>
#include <vector>

#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Camera/camera.h"
#include "../Math/Collisions.h"
#include "../Meshes/Meshes.h"
#include "../Scene/SceneManager.h"

class Car {
public:
    bool spawn(const std::vector<MeshData> &meshes, Camera &camera, Collisions &collisions,
               const ModelAnimationData *animationData, bool animate);

    void despawn(Collisions &collisions);

    void drive(float throttle, bool braking, float steering, float deltaTime,
               bool shiftUp, bool shiftDown);

    void updateTransform();
    void updateAnimation(float deltaTime);

    void updateCamera(Camera &camera) const;

    bool isActive() const;

    float getRPM() const;

    float getSpeedKmh() const;

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
    float steeringInput = 0.0f;
    float wheelRotation = 0.0f;
    float wheelRadius = 0.48f;
    std::array<glm::vec3, 4> wheelCenters{};
    glm::vec3 steeringWheelPivot{0.0f};
    float animationTime = 0.0f;
    bool skeletalAnimationEnabled = false;
    ModelAnimationData animationData;
    int gear = 1;
    btRigidBody *body = nullptr;
    btVehicleRaycaster *vehicleRaycaster = nullptr;
    btRaycastVehicle *vehicle = nullptr;
    SceneObject physicsObject;
    std::vector<SceneObject> renderObjects;
};

#endif
