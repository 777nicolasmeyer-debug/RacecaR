#include "Car.h"

#include "CarConfig.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
    glm::vec3 getModelCenter(const std::vector<MeshData> &meshes) {
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        bool foundVertex = false;

        for (const auto &mesh: meshes) {
            for (size_t i = 0; i + 2 < mesh.vertices.size(); i += 8) {
                const glm::vec3 point(mesh.vertices[i], mesh.vertices[i + 1],
                                      mesh.vertices[i + 2]);
                minimum = glm::min(minimum, point);
                maximum = glm::max(maximum, point);
                foundVertex = true;
            }
        }

        return foundVertex ? (minimum + maximum) * 0.5f : glm::vec3(0.0f);
    }

    MeshData makeCollisionBox() {
        MeshData mesh;
        mesh.indexType = GL_UNSIGNED_INT;
        for (const float x: {-car_config::chassisHalfExtents.x, car_config::chassisHalfExtents.x}) {
            for (const float y: {-car_config::chassisHalfExtents.y, car_config::chassisHalfExtents.y}) {
                for (const float z: {-car_config::chassisHalfExtents.z, car_config::chassisHalfExtents.z}) {
                    mesh.vertices.insert(mesh.vertices.end(), {x, y, z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f});
                }
            }
        }
        return mesh;
    }
}

bool Car::spawn(const std::vector<MeshData> &meshes, Camera &camera, Collisions &collisions) {
    if (meshes.empty()) {
        std::cerr << "Cannot enter player mode: car model has no renderable meshes" << std::endl;
        return false;
    }

    const glm::vec3 modelCenter = getModelCenter(meshes);
    glm::vec3 initialForward = camera.getFront();
    initialForward.y = 0.0f;
    const float initialYaw = glm::dot(initialForward, initialForward) > 0.0f
                                 ? std::atan2(initialForward.x, initialForward.z)
                                 : 0.0f;
    rotation = glm::angleAxis(initialYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    camera.setDirection(rotation * glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 carModelPosition = camera.getPosition() - rotation * car_config::cockpitOffset;
    position = carModelPosition + rotation * car_config::chassisOffset;
    physicsObject.position = position;
    physicsObject.rotation = rotation;
    physicsObject.scale = glm::vec3(1.0f);

    renderObjects.clear();
    renderObjects.reserve(meshes.size());
    for (const auto &sourceMesh: meshes) {
        MeshData mesh = sourceMesh;
        for (size_t i = 0; i + 2 < mesh.vertices.size(); i += 8) {
            mesh.vertices[i] -= modelCenter.x;
            mesh.vertices[i + 1] -= modelCenter.y;
            mesh.vertices[i + 2] -= modelCenter.z;
        }

        SceneObject object;
        object.buffers.init(mesh);
        object.position = position - rotation * car_config::chassisOffset;
        object.rotation = rotation;
        object.textureID = mesh.textureID;
        object.baseColor = mesh.baseColor;
        object.localCenter = mesh.localCenter - modelCenter;
        object.transparent = mesh.transparent;
        object.alphaMasked = mesh.alphaMasked;
        object.alphaCutoff = mesh.alphaCutoff;
        renderObjects.push_back(object);
    }

    body = collisions.boxShapeD(physicsObject, makeCollisionBox());
    if (body == nullptr) {
        std::cerr << "Failed to spawn car body" << std::endl;
        renderObjects.clear();
        return false;
    }
    body->setAngularFactor(btVector3(1.0f, 1.0f, 1.0f));
    body->setFriction(0.8f);
    body->setRestitution(0.0f);
    body->setDamping(0.05f, 0.85f);
    rpm = car_config::idleRPM;
    gear = 1;
    return true;
}

void Car::despawn(Collisions &collisions) {
    if (body != nullptr) {
        collisions.removeRigidBody(body);
        body = nullptr;
    }
    physicsObject.body = nullptr;
    renderObjects.clear();
}

void Car::drive(const float throttle, const bool braking, const float steering,
                const float deltaTime, const glm::vec3 &groundNormal,
                const bool shiftUp, const bool shiftDown) {
    if (body == nullptr) {
        return;
    }

    // Limit frame-time spikes so a single slow frame cannot apply a huge control step.
    const float step = std::clamp(deltaTime, 0.0f, 0.1f);
    const bool grounded = groundNormal.y > 0.2f;
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    const btVector3 velocity = body->getLinearVelocity();
    const float horizontalSpeed =
            std::sqrt(velocity.x() * velocity.x() + velocity.z() * velocity.z());
    constexpr int gearCount = static_cast<int>(car_config::gearMaxSpeeds.size());
    gear = std::clamp(gear, 1, gearCount);

    // Shift only when exactly one shift key is pressed; clamp to the available gears.
    if (shiftUp != shiftDown) {
        gear = std::clamp(gear + (shiftUp ? 1 : -1), 1, gearCount);
    }

    // RPM is a speed/load estimate for the dashboard and sound system, not a force calculation.
    const int gearIndex = gear - 1;
    const float gearMaxSpeed = car_config::gearMaxSpeeds[gearIndex];
    const float gearSpeedFactor = std::clamp(horizontalSpeed / gearMaxSpeed, 0.0f, 1.0f);
    const float rpmLoad =
            std::clamp(gearSpeedFactor + throttle * 0.15f, 0.0f, 1.0f);
    const float targetRPM =
            car_config::idleRPM + (car_config::redlineRPM - car_config::idleRPM) * rpmLoad;
    const float rpmResponse =
            std::min(step * 5.0f * car_config::gearRpmMultiplier[gearIndex], 1.0f);
    rpm += (targetRPM - rpm) * rpmResponse;

    const float downforce =
            car_config::downforceCoefficient * horizontalSpeed * horizontalSpeed;
    body->applyCentralForce(btVector3(0.0f, -downforce, 0.0f));

    // Keep the drive direction aligned to the road surface while grounded.
    if (grounded) {
        forward -= groundNormal * glm::dot(forward, groundNormal);
    } else {
        forward.y = 0.0f;
    }
    if (glm::dot(forward, forward) > 0.0001f) {
        forward = glm::normalize(forward);
    }
    const glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    const float mass = static_cast<float>(body->getMass());
    const float gripLimit = (mass * 9.81f + downforce) * car_config::tireGrip;
    const float forwardSpeed = forward.x * velocity.x() + forward.z * velocity.z();
    const float lateralSpeed = right.x * velocity.x() + right.z * velocity.z();
    const float requestedLateralForce = -lateralSpeed * mass * 8.0f;
    const float lateralForce = std::clamp(requestedLateralForce, -gripLimit, gripLimit);
    float longitudinalBrakeForce = 0.0f;

    // Braking opposes motion and shares tire grip with cornering; throttle targets gear speed.
    if (braking) {
        if (grounded && step > 0.0f && std::abs(forwardSpeed) > 0.01f) {
                const float availableBrakeGrip =
                    std::sqrt(std::max(gripLimit * gripLimit - lateralForce * lateralForce, 0.0f));
                const float maximumBrakeForce =
                    mass * 9.81f * car_config::brakeForceCoefficient;
                const float stoppingForce = mass * std::abs(forwardSpeed) / step;
                longitudinalBrakeForce =
                    std::min({maximumBrakeForce, stoppingForce, availableBrakeGrip});
                const float brakeDirection = forwardSpeed > 0.0f ? -1.0f : 1.0f;
                body->applyCentralForce(btVector3(
                    forward.x * brakeDirection * longitudinalBrakeForce, 0.0f,
                    forward.z * brakeDirection * longitudinalBrakeForce));
        }
    } else if (throttle > 0.0f) {
        const float maximumForwardAcceleration =
                8.0f * car_config::gearAcceleration[gearIndex] *
                car_config::gearRpmMultiplier[gearIndex];
        const float targetForwardSpeed =
                std::clamp(forwardSpeed + maximumForwardAcceleration * step, -6.0f, gearMaxSpeed);
        const float speedChange = targetForwardSpeed - forwardSpeed;
        body->setLinearVelocity(btVector3(velocity.x() + forward.x * speedChange,
                                          velocity.y(),
                                          velocity.z() + forward.z * speedChange));
    } else {
        const float coastFactor = std::exp(-car_config::coastDrag * step);
        body->setLinearVelocity(btVector3(velocity.x() * coastFactor, velocity.y(),
                                          velocity.z() * coastFactor));
    }

    // Dampen roll/pitch and steer by adjusting yaw rate; reverse flips steering direction.
    btVector3 angularVelocity = body->getAngularVelocity();
    angularVelocity.setX(angularVelocity.x() * std::exp(-2.0f * step));
    angularVelocity.setZ(angularVelocity.z() * std::exp(-2.0f * step));
    const btVector3 worldUp(0.0f, 1.0f, 0.0f);
    btVector3 targetUp = grounded
                             ? btVector3(groundNormal.x, groundNormal.y, groundNormal.z)
                             : worldUp;
    if (grounded) {
        targetUp.normalize();
    }
    const btVector3 carUp =
            body->getWorldTransform().getBasis() * btVector3(0.0f, 1.0f, 0.0f);
    if (carUp.dot(targetUp) < btScalar(0.8660254f)) {
        body->applyTorque(carUp.cross(targetUp) * btScalar(35.0f));
    }
    constexpr btScalar maxRollPitchRate = btScalar(2.5f);
    angularVelocity.setX(btClamped(angularVelocity.x(), -maxRollPitchRate, maxRollPitchRate));
    angularVelocity.setZ(btClamped(angularVelocity.z(), -maxRollPitchRate, maxRollPitchRate));
    const float steeringDirection = forwardSpeed < -0.1f ? -1.0f : 1.0f;
    const float targetYawRate =
            -steering * car_config::maxYawRate * gearSpeedFactor * steeringDirection;
    const float steeringResponse = std::min(step * 8.0f, 1.0f);
    angularVelocity.setY(angularVelocity.y() +
                         (targetYawRate - angularVelocity.y()) * steeringResponse);
    body->setAngularVelocity(angularVelocity);

    // Apply the remaining tire grip laterally to reduce sliding without exceeding traction.
    if (grounded) {
        const float availableLateralGrip = std::sqrt(std::max(
            gripLimit * gripLimit - longitudinalBrakeForce * longitudinalBrakeForce, 0.0f));
        const float corneringForce =
                std::clamp(requestedLateralForce, -availableLateralGrip, availableLateralGrip);
        body->applyCentralForce(
            btVector3(right.x * corneringForce, 0.0f, right.z * corneringForce));
    }
    body->activate(true);
}

void Car::updateTransform() {
    if (body == nullptr || body->getMotionState() == nullptr) {
        return;
    }

    btTransform transform;
    body->getMotionState()->getWorldTransform(transform);
    position = glm::vec3(transform.getOrigin().x(), transform.getOrigin().y(),
                         transform.getOrigin().z());
    const btQuaternion bodyRotation = transform.getRotation();
    rotation = glm::quat(bodyRotation.w(), bodyRotation.x(), bodyRotation.y(), bodyRotation.z());
    physicsObject.position = position;
    physicsObject.rotation = rotation;

    for (auto &object: renderObjects) {
        object.position = position - rotation * car_config::chassisOffset;
        object.rotation = rotation;
    }
}

void Car::updateCamera(Camera &camera) const {
    if (body != nullptr) {
        camera.setPosition(position - rotation * car_config::chassisOffset +
                           rotation * car_config::cockpitOffset);
    }
}

bool Car::isActive() const {
    return body != nullptr;
}

float Car::getRPM() const {
    return rpm;
}

int Car::getGear() const {
    return gear;
}

const glm::vec3 &Car::getPosition() const {
    return position;
}

const glm::quat &Car::getRotation() const {
    return rotation;
}

btRigidBody *Car::getBody() const {
    return body;
}

std::vector<SceneObject> &Car::getRenderObjects() {
    return renderObjects;
}

const std::vector<SceneObject> &Car::getRenderObjects() const {
    return renderObjects;
}
