#include "Engine.h"

#include <iostream>

void Engine::handleKeyboardInput(const CameraInput &input, const float deltaTime) {
    const bool shiftUp = input.shiftUp && !shiftUpWasPressed;
    const bool shiftDown = input.shiftDown && !shiftDownWasPressed;
    shiftUpWasPressed = input.shiftUp;
    shiftDownWasPressed = input.shiftDown;

    if (!car.isActive()) {
        if (input.forward) camera.moveForward(cameraSpeed, deltaTime);
        if (input.backward) camera.moveBackward(cameraSpeed, deltaTime);
        if (input.left) camera.moveLeft(cameraSpeed, deltaTime);
        if (input.right) camera.moveRight(cameraSpeed, deltaTime);
        if (input.up) camera.moveUp(cameraSpeed, deltaTime);
        if (input.down) camera.moveDown(cameraSpeed, deltaTime);
        return;
    }

    const float throttle = static_cast<float>(input.forward);
    const bool braking = input.backward || input.brake;
    const float steering = static_cast<float>(input.right) - static_cast<float>(input.left);
    car.drive(throttle, braking, steering, deltaTime,
              collisions.getGroundNormal(car.getBody()), shiftUp, shiftDown);
}

void Engine::handleMouseInput(const double xpos, const double ypos) {
    camera.mouseInput(xpos, ypos);
}

float Engine::getPlayerRPM() const {
    return car.isActive() ? car.getRPM() : 0.0f;
}

int Engine::getPlayerGear() const {
    return car.isActive() ? car.getGear() : 0;
}

void Engine::togglePlayer() {
    if (car.isActive()) {
        car.despawn(collisions);
        return;
    }

    const auto &carMeshes = loadModelMeshes(ASSET_RESOURCES_PATH "RB18_F1.glb");
    car.spawn(carMeshes, camera, collisions);
}

void Engine::updatePhysicsTransforms() {
    const auto update = [](SceneObject &object) {
        if (object.body == nullptr || object.body->getMotionState() == nullptr) {
            return;
        }
        btTransform transform;
        object.body->getMotionState()->getWorldTransform(transform);
        object.position = glm::vec3(transform.getOrigin().x(), transform.getOrigin().y(),
                                    transform.getOrigin().z());
        const btQuaternion rotation = transform.getRotation();
        object.rotation = glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z());
    };
    for (auto &object: sceneObjects) update(object);
    for (auto &object: terrainObjects) update(object);
    car.updateTransform();
}

bool Engine::hasPlayer() const {
    return car.isActive();
}
