#include "Car.h"

#include "CarConfig.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <functional>
#include <iostream>
#include <limits>
#include <string>

namespace {
    glm::quat interpolateRotation(glm::quat from, glm::quat to, const float factor) {
        float cosine = glm::dot(from, to);
        if (cosine < 0.0f) {
            to = -to;
            cosine = -cosine;
        }
        if (cosine > 0.9995f) {
            return glm::normalize(from * (1.0f - factor) + to * factor);
        }
        const float angle = std::acos(std::clamp(cosine, -1.0f, 1.0f));
        const float sine = std::sin(angle);
        return glm::normalize(from * (std::sin((1.0f - factor) * angle) / sine) +
                              to * (std::sin(factor * angle) / sine));
    }

    std::string lowercase(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return text;
    }

    bool isFrontWheelPart(const std::string &name) {
        return name.find("front tire") != std::string::npos ||
               name.find("rim front") != std::string::npos ||
               name.find("inside rim front") != std::string::npos ||
               name.find("front disk") != std::string::npos ||
               name.find("bujon front wheel") != std::string::npos ||
               name.find("bujon front weel") != std::string::npos;
    }

    bool isRollingWheelPart(const std::string &name) {
        return isFrontWheelPart(name) ||
               name.find("back tire") != std::string::npos ||
               name.find("rim bck") != std::string::npos ||
               name.find("rim back") != std::string::npos ||
               name.find("inside rim back") != std::string::npos ||
               name.find("bujon back whell") != std::string::npos ||
               name.find("bujon back wheel") != std::string::npos;
    }

    bool isTirePart(const std::string &name) {
        return name.find("front tire") != std::string::npos ||
               name.find("back tire") != std::string::npos;
    }

    int wheelIndexForPart(const std::string &name, const glm::vec3 &center,
                         const std::array<glm::vec3, 4> &wheelCenters) {
        if (!isRollingWheelPart(lowercase(name))) {
            return -1;
        }
        int closestIndex = -1;
        float closestDistance = std::numeric_limits<float>::max();
        for (size_t index = 0; index < wheelCenters.size(); ++index) {
            const glm::vec3 difference = center - wheelCenters[index];
            const float distance = glm::dot(difference, difference);
            if (distance < closestDistance) {
               closestDistance = distance;
               closestIndex = static_cast<int>(index);
            }
        }
        return closestIndex;
    }

    void orderWheelPair(std::vector<glm::vec3> &wheels) {
        if (wheels.size() != 2) {
            return;
        }
        const glm::vec3 difference = glm::abs(wheels[0] - wheels[1]);
        const bool separatedOnX = difference.x >= difference.z;
        const float firstSide = separatedOnX ? wheels[0].x : wheels[0].z;
        const float secondSide = separatedOnX ? wheels[1].x : wheels[1].z;
        if (firstSide > secondSide) {
            std::swap(wheels[0], wheels[1]);
        }
    }

    bool isSteeringWheel(const std::string &name) {
        return name == "control" || name.find("steering wheel") != std::string::npos;
    }

    bool isSteeringWheelControl(const std::string &name) {
        return name == "control shaft" || name == "shifter" || name == "switch";
    }

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

bool Car::spawn(const std::vector<MeshData> &meshes, Camera &camera, Collisions &collisions,
                const ModelAnimationData *sourceAnimation, const bool animate) {
    if (meshes.empty()) {
        std::cerr << "Cannot enter player mode: car model has no renderable meshes" << std::endl;
        return false;
    }

    const glm::vec3 modelCenter = getModelCenter(meshes);
    std::vector<glm::vec3> frontWheelCenters;
    std::vector<glm::vec3> rearWheelCenters;
    steeringWheelPivot = glm::vec3(0.0f);
    wheelRadius = 0.0f;
    for (const auto &mesh : meshes) {
        const std::string name = lowercase(mesh.nodeName);
        if (isSteeringWheel(name)) {
            steeringWheelPivot = mesh.localCenter - modelCenter;
        }
        if (!isTirePart(name)) {
            continue;
        }
        float tireRadius = 0.0f;
        for (size_t vertex = 0; vertex + 2 < mesh.vertices.size(); vertex += 8) {
            const float y = mesh.vertices[vertex + 1] - mesh.localCenter.y;
            const float z = mesh.vertices[vertex + 2] - mesh.localCenter.z;
            tireRadius = std::max(tireRadius, std::sqrt(y * y + z * z));
        }
        if (tireRadius > 0.1f) {
            wheelRadius = std::max(wheelRadius, tireRadius);
        }
        const glm::vec3 center = mesh.localCenter - modelCenter;
        (name.find("front tire") != std::string::npos ? frontWheelCenters
                                                       : rearWheelCenters)
            .push_back(center);
    }
    if (frontWheelCenters.size() == 2 && rearWheelCenters.size() == 2) {
        orderWheelPair(frontWheelCenters);
        orderWheelPair(rearWheelCenters);
        wheelCenters = {frontWheelCenters[0], frontWheelCenters[1],
                        rearWheelCenters[0], rearWheelCenters[1]};
    } else {
        wheelCenters = {glm::vec3(-0.82f, -0.02f, 1.40f),
                        glm::vec3(0.82f, -0.02f, 1.40f),
                        glm::vec3(-0.82f, -0.02f, -1.40f),
                        glm::vec3(0.82f, -0.02f, -1.40f)};
    }
    if (wheelRadius <= 0.0f) {
        wheelRadius = car_config::wheelRadius;
    }
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
    steeringInput = 0.0f;
    wheelRotation = 0.0f;
    animationTime = 0.0f;
    animationData = sourceAnimation != nullptr ? *sourceAnimation : ModelAnimationData{};
    const bool modelHasSkinnedMesh =
        std::any_of(meshes.begin(), meshes.end(), [](const MeshData &mesh) {
            return mesh.skinned;
        });
    skeletalAnimationEnabled =
        animate && modelHasSkinnedMesh && !animationData.nodes.empty() &&
        car_config::skeletalAnimationClip < animationData.clips.size();
    if (animate && !skeletalAnimationEnabled) {
        std::cerr << "Skeletal animation enabled, but the car model has no supported skin and clip"
                  << std::endl;
    }
    for (const auto &sourceMesh: meshes) {
        MeshData mesh = sourceMesh;
        for (size_t i = 0; i + 2 < mesh.vertices.size(); i += 8) {
            mesh.vertices[i] -= modelCenter.x;
            mesh.vertices[i + 1] -= modelCenter.y;
            mesh.vertices[i + 2] -= modelCenter.z;
        }

        SceneObject object;
        object.buffers.init(mesh);
        object.position = position - rotation * car_config::chassisOffset +
                          rotation * car_config::visualChassisDrop;
        object.rotation = rotation;
        object.nodeName = mesh.nodeName;
        object.skinningEnabled = skeletalAnimationEnabled && mesh.skinned;
        object.skinJointNodes = mesh.skinJointNodes;
        object.inverseBindMatrices = mesh.inverseBindMatrices;
        object.inverseMeshNodeTransform = mesh.inverseMeshNodeTransform;
        object.jointMatrices.assign(mesh.skinJointNodes.size(), glm::mat4(1.0f));
        object.textureID = mesh.textureID;
        object.baseColor = mesh.baseColor;
        object.localCenter = mesh.localCenter - modelCenter;
        object.transparent = mesh.transparent;
        object.alphaMasked = mesh.alphaMasked;
        object.alphaCutoff = mesh.alphaCutoff;
        renderObjects.push_back(object);
    }

    body = collisions.boxShapeD(physicsObject, makeCollisionBox(), glm::vec3(1.0f),
                                car_config::chassisMass);
    if (body == nullptr) {
        std::cerr << "Failed to spawn car body" << std::endl;
        renderObjects.clear();
        return false;
    }
    body->setAngularFactor(btVector3(1.0f, 1.0f, 1.0f));
    body->setFriction(car_config::bodyFriction);
    body->setRestitution(car_config::bodyRestitution);
    body->setDamping(car_config::bodyLinearDamping, car_config::bodyAngularDamping);

    vehicleRaycaster = collisions.createVehicleRaycaster();
    btRaycastVehicle::btVehicleTuning tuning;
    tuning.m_suspensionStiffness = car_config::suspensionStiffness;
    tuning.m_suspensionCompression = car_config::suspensionCompression;
    tuning.m_suspensionDamping = car_config::suspensionDamping;
    tuning.m_maxSuspensionTravelCm = car_config::suspensionMaxTravelCm;
    tuning.m_maxSuspensionForce = car_config::suspensionMaxForce;
    vehicle = new btRaycastVehicle(tuning, body, vehicleRaycaster);
    vehicle->setCoordinateSystem(0, 1, 2);
    collisions.addVehicle(vehicle);
    for (size_t index = 0; index < wheelCenters.size(); ++index) {
        const glm::vec3 connection = wheelCenters[index] -
                                     car_config::chassisOffset +
                                     glm::vec3(0.0f, car_config::suspensionRestLength -
                                                         car_config::chassisRideHeightOffset,
                                               0.0f);
        btWheelInfo &wheel = vehicle->addWheel(
            btVector3(connection.x, connection.y, connection.z),
            btVector3(0.0f, -1.0f, 0.0f), btVector3(-1.0f, 0.0f, 0.0f),
            car_config::suspensionRestLength, wheelRadius, tuning, index < 2);
        wheel.m_frictionSlip = car_config::tireGrip;
        wheel.m_rollInfluence = car_config::wheelRollInfluence;
        wheel.m_maxSuspensionForce = car_config::suspensionMaxForce;
    }
    body->setActivationState(DISABLE_DEACTIVATION);

    rpm = car_config::idleRPM;
    gear = 1;
    return true;
}

void Car::despawn(Collisions &collisions) {
    if (vehicle != nullptr) {
        collisions.removeVehicle(vehicle);
        delete vehicle;
        vehicle = nullptr;
    }
    delete vehicleRaycaster;
    vehicleRaycaster = nullptr;
    if (body != nullptr) {
        collisions.removeRigidBody(body);
        body = nullptr;
    }
    physicsObject.body = nullptr;
    renderObjects.clear();
    animationData = {};
    skeletalAnimationEnabled = false;
}

void Car::drive(const float throttle, const bool braking, const float steering,
                const float deltaTime, const bool shiftUp, const bool shiftDown) {
    if (body == nullptr || vehicle == nullptr) {
        return;
    }

    const float step = std::clamp(deltaTime, 0.0f, car_config::maxDriveStep);
    const float targetSteering = std::clamp(steering, -1.0f, 1.0f);
    const float maximumSteeringChange = car_config::steeringRampRate * step;
    steeringInput += std::clamp(targetSteering - steeringInput,
                                -maximumSteeringChange, maximumSteeringChange);
    const btVector3 velocity = body->getLinearVelocity();
    const float horizontalSpeed =
            std::sqrt(velocity.x() * velocity.x() + velocity.z() * velocity.z());
    constexpr int gearCount = static_cast<int>(car_config::gearMaxSpeeds.size());
    gear = std::clamp(gear, 1, gearCount);

    // Shift only when exactly one shift key is pressed; clamp to the available gears.
    if (shiftUp != shiftDown) {
        gear = std::clamp(gear + (shiftUp ? 1 : -1), 1, gearCount);
    }

    // Couple engine speed to wheel speed through the selected gear and final drive.
    const int gearIndex = gear - 1;
    const float gearMaxSpeed = car_config::gearMaxSpeeds[gearIndex];
    const float steeringSpeedFactor =
        std::clamp(horizontalSpeed / car_config::highSpeedSteeringStart, 0.0f, 1.0f);
    const float steeringAngle =
        car_config::frontWheelSteeringAngle * (1.0f - steeringSpeedFactor) +
        car_config::highSpeedSteeringAngle * steeringSpeedFactor;
    vehicle->setSteeringValue(-steeringInput * steeringAngle, 0);
    vehicle->setSteeringValue(-steeringInput * steeringAngle, 1);

    constexpr float twoPi = 6.28318530718f;
    const float wheelRPM =
            horizontalSpeed * car_config::worldSpeedToMetersPerSecond * 60.0f /
            (twoPi * wheelRadius);
    const float coupledRPM = std::clamp(
        wheelRPM * car_config::gearRatios[gearIndex] * car_config::finalDriveRatio,
        car_config::idleRPM, car_config::redlineRPM);
    const float clutchEngagement =
            std::clamp(horizontalSpeed * car_config::worldSpeedToMetersPerSecond /
                           car_config::clutchEngagementSpeed,
                       0.0f, 1.0f);
    const float freeRevRPM =
            car_config::idleRPM +
            std::clamp(throttle, 0.0f, 1.0f) *
                (car_config::redlineRPM - car_config::idleRPM);
    const float targetRPM = std::max(
        coupledRPM, coupledRPM + (freeRevRPM - coupledRPM) * (1.0f - clutchEngagement));
    const float responseRate =
            targetRPM > rpm ? car_config::rpmRiseRate : car_config::rpmFallRate;
    const float rpmResponse = std::min(step * responseRate, 1.0f);
    rpm += (targetRPM - rpm) * rpmResponse;

    const float speedMetersPerSecond =
        horizontalSpeed * car_config::worldSpeedToMetersPerSecond;
    const float downforce = car_config::downforceCoefficient *
                            speedMetersPerSecond * speedMetersPerSecond;
    body->applyCentralForce(btVector3(0.0f, -downforce, 0.0f));

    const float forwardSpeed = velocity.dot(vehicle->getForwardVector());
    const float inverseMass = body->getInvMass();
    const float mass = inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
    const float engineSpeedFactor =
        std::clamp(1.0f - forwardSpeed / gearMaxSpeed, 0.0f, 1.0f);
    const float totalEngineForce = braking ? 0.0f :
        mass * car_config::driveAccelerationScale *
        car_config::gearAcceleration[gearIndex] *
        car_config::gearRpmMultiplier[gearIndex] * engineSpeedFactor *
        std::clamp(throttle, 0.0f, 1.0f);
    for (int index = 0; index < vehicle->getNumWheels(); ++index) {
        const bool drivenWheel = index >= 2;
        vehicle->applyEngineForce(drivenWheel ? totalEngineForce * 0.5f : 0.0f, index);
        const float brakeForce = braking
            ? mass * car_config::gravity * car_config::brakeForceCoefficient * 0.25f
            : (throttle <= 0.0f ? car_config::coastBrakeForce : 0.0f);
        vehicle->setBrake(brakeForce, index);
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
        object.position = position - rotation * car_config::chassisOffset +
                          rotation * car_config::visualChassisDrop;
        object.rotation = rotation;

        if (skeletalAnimationEnabled) {
            continue;
        }
        const std::string nodeName = lowercase(object.nodeName);
        const int wheelIndex =
            wheelIndexForPart(object.nodeName, object.localCenter, wheelCenters);
        if (vehicle != nullptr && wheelIndex >= 0) {
            vehicle->updateWheelTransform(wheelIndex, true);
            const btTransform &wheelTransform =
                vehicle->getWheelInfo(wheelIndex).m_worldTransform;
            const btQuaternion wheelRotation = wheelTransform.getRotation();
            object.rotation = glm::quat(wheelRotation.w(), wheelRotation.x(),
                                         wheelRotation.y(), wheelRotation.z());
            const btVector3 wheelPosition = wheelTransform.getOrigin();
            object.position =
                glm::vec3(wheelPosition.x(), wheelPosition.y(), wheelPosition.z()) -
                object.rotation * object.localCenter;
            object.localAnimationRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            continue;
        }
        const glm::vec3 steeringPartOffset = object.localCenter - steeringWheelPivot;
        const bool attachedSteeringControl =
            isSteeringWheelControl(nodeName) &&
            glm::dot(steeringPartOffset, steeringPartOffset) <= 0.64f;
        if (isSteeringWheel(nodeName) || attachedSteeringControl) {
            const glm::quat steeringRotation =
                glm::angleAxis(steeringInput * car_config::steeringWheelAngle,
                               glm::vec3(0.0f, 0.0f, 1.0f));
            object.position =
                position - rotation * car_config::chassisOffset +
                rotation * (steeringWheelPivot - steeringRotation * steeringWheelPivot);
            object.rotation = rotation * steeringRotation;
            object.localAnimationRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            continue;
        }
        glm::quat animation(1.0f, 0.0f, 0.0f, 0.0f);
        if (isFrontWheelPart(nodeName)) {
            animation = glm::angleAxis(steeringInput * car_config::frontWheelSteeringAngle,
                                       glm::vec3(0.0f, 1.0f, 0.0f));
        }
        object.localAnimationRotation = animation;
    }
}

void Car::updateAnimation(const float deltaTime) {
    if (!skeletalAnimationEnabled || !isActive() ||
        car_config::skeletalAnimationClip >= animationData.clips.size()) {
        return;
    }

    const auto &clip = animationData.clips[car_config::skeletalAnimationClip];
    if (clip.duration > 0.0f) {
        animationTime = std::fmod(animationTime + std::max(deltaTime, 0.0f), clip.duration);
    }

    std::vector<AnimationNodeData> pose = animationData.nodes;
    for (const auto &channel : clip.channels) {
        if (channel.node < 0 || static_cast<size_t>(channel.node) >= pose.size() ||
            channel.times.empty()) {
            continue;
        }
        const auto upper = std::upper_bound(channel.times.begin(), channel.times.end(), animationTime);
        const size_t next = upper == channel.times.end()
                                ? channel.times.size() - 1
                                : static_cast<size_t>(upper - channel.times.begin());
        const size_t previous = next == 0 ? 0 : next - 1;
        float factor = 0.0f;
        if (channel.interpolation != "STEP" && next != previous) {
            const float span = channel.times[next] - channel.times[previous];
            factor = span > 0.0f
                         ? std::clamp((animationTime - channel.times[previous]) / span, 0.0f, 1.0f)
                         : 0.0f;
        }
        const size_t components =
            channel.path == AnimationPath::Rotation ? 4 : 3;
        const size_t firstOffset = previous * components;
        const size_t secondOffset = next * components;
        if (secondOffset + components > channel.values.size()) {
            continue;
        }
        const auto value = [&](const size_t component) {
            return channel.values[firstOffset + component] * (1.0f - factor) +
                   channel.values[secondOffset + component] * factor;
        };
        auto &node = pose[channel.node];
        node.usesMatrix = false;
        if (channel.path == AnimationPath::Translation) {
            node.translation = glm::vec3(value(0), value(1), value(2));
        } else if (channel.path == AnimationPath::Scale) {
            node.scale = glm::vec3(value(0), value(1), value(2));
        } else {
            const glm::quat from(channel.values[firstOffset + 3],
                                 channel.values[firstOffset + 0],
                                 channel.values[firstOffset + 1],
                                 channel.values[firstOffset + 2]);
            const glm::quat to(channel.values[secondOffset + 3],
                               channel.values[secondOffset + 0],
                               channel.values[secondOffset + 1],
                               channel.values[secondOffset + 2]);
            node.rotation = interpolateRotation(from, to, factor);
        }
    }

    std::vector<glm::mat4> worldMatrices(pose.size(), glm::mat4(1.0f));
    std::vector<bool> calculated(pose.size(), false);
    std::function<glm::mat4(size_t)> calculateWorld = [&](const size_t index) {
        if (calculated[index]) {
            return worldMatrices[index];
        }
        const auto &node = pose[index];
        glm::mat4 local = node.matrix;
        if (!node.usesMatrix) {
            local = glm::translate(glm::mat4(1.0f), node.translation) *
                    glm::mat4_cast(node.rotation) *
                    glm::scale(glm::mat4(1.0f), node.scale);
        }
        if (node.parent >= 0 && static_cast<size_t>(node.parent) < pose.size()) {
            local = calculateWorld(static_cast<size_t>(node.parent)) * local;
        }
        worldMatrices[index] = local;
        calculated[index] = true;
        return local;
    };
    for (size_t node = 0; node < pose.size(); ++node) {
        calculateWorld(node);
    }

    for (auto &object : renderObjects) {
        if (!object.skinningEnabled) {
            continue;
        }
        const size_t jointCount = std::min(
            object.skinJointNodes.size(), object.inverseBindMatrices.size());
        object.jointMatrices.resize(jointCount, glm::mat4(1.0f));
        for (size_t joint = 0; joint < jointCount; ++joint) {
            const int node = object.skinJointNodes[joint];
            if (node < 0 || static_cast<size_t>(node) >= worldMatrices.size()) {
                object.jointMatrices[joint] = glm::mat4(1.0f);
                continue;
            }
            object.jointMatrices[joint] =
                worldMatrices[node] * object.inverseBindMatrices[joint] *
                object.inverseMeshNodeTransform;
        }
    }
}

void Car::updateCamera(Camera &camera) const {
    if (body != nullptr) {
        camera.setPosition(position - rotation * car_config::chassisOffset +
                           rotation * car_config::cockpitOffset +
                           rotation * car_config::visualChassisDrop);
    }
}

bool Car::isActive() const {
    return body != nullptr;
}

float Car::getRPM() const {
    return rpm;
}

float Car::getSpeedKmh() const {
    if (body == nullptr) {
        return 0.0f;
    }

    const btVector3 velocity = body->getLinearVelocity();
    return std::sqrt(velocity.x() * velocity.x() + velocity.z() * velocity.z()) *
           car_config::worldSpeedToMetersPerSecond * car_config::metersPerSecondToKmh;
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
