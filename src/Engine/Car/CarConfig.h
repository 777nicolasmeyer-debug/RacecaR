#ifndef RACECAR_CAR_CONFIG_H
#define RACECAR_CAR_CONFIG_H

#include <array>
#include <cstddef>

#include <glm/glm.hpp>

namespace car_config {
    inline constexpr bool skeletalAnimationEnabled = false;
    inline constexpr size_t skeletalAnimationClip = 0;

    // Car model offsets and collision-box dimensions.
    inline const glm::vec3 cockpitOffset{0.0f, 0.50f, 0.057f};
    inline const glm::vec3 chassisOffset{0.0f, -0.3f, 0.0f};
    inline const glm::vec3 visualChassisDrop{0.0f};
    inline const glm::vec3 chassisHalfExtents{0.72f, 0.22f, 1.55f};
    inline constexpr float chassisMass = 800.0f;

    // Wheel centers use the standard vehicle frame: +X right, +Y up, +Z forward.
    // Speed conversion: world units to meters per second, then to km/h.
    inline constexpr float worldSpeedToMetersPerSecond = 2.0f;
    inline constexpr float metersPerSecondToKmh = 3.6f;

    // Engine RPM, wheel drivetrain, and clutch response.
    inline constexpr float idleRPM = 1500.0f;
    inline constexpr float redlineRPM = 15000.0f;
    inline constexpr float frontWheelSteeringAngle = 0.22f;
    inline constexpr float highSpeedSteeringAngle = 0.08f;
    inline constexpr float highSpeedSteeringStart = 20.0f;
    inline constexpr float steeringRampRate = 4.0f;
    inline constexpr float steeringWheelAngle = 2.0f;
    inline constexpr float finalDriveRatio = 3.8f;
    inline constexpr float clutchEngagementSpeed = 8.0f;
    inline constexpr float rpmRiseRate = 6.0f;
    inline constexpr float rpmFallRate = 4.0f;

    // Per-gear top speed, acceleration strength, RPM response, and gearbox ratios.
    inline constexpr std::array<float, 8> gearMaxSpeeds{
        22.0f, 34.0f, 47.5f, 55.5f, 67.5f, 79.0f, 91.5f, 102.0f
    };
    inline constexpr std::array<float, 8> gearRatios{
        4.184f, 2.707f, 1.938f, 1.659f, 1.361f, 1.164f, 1.006f, 0.902f
    };
    inline constexpr std::array<float, 8> gearAcceleration{
        2.5f, 2.0f, 1.78f, 1.68f, 1.62f, 1.58f, 1.54f, 1.50f
    };
    inline constexpr std::array<float, 8> gearRpmMultiplier{
        1.1f, 1.08f, 1.06f, 1.04f, 1.02f, 1.0f, 0.98f, 0.96f
    };

    // Drive force, gravity, and timestep.
    inline constexpr float driveAccelerationScale = 20.0f;
    inline constexpr float reverseSpeedLimit = -6.0f;
    inline constexpr float coastDrag = 0.12f;
    inline constexpr float gravity = 9.81f;
    inline constexpr float maxDriveStep = 0.1f;

    // Downforce and braking strength.
    inline constexpr float downforceCoefficient = 3.5f;
    inline constexpr float brakeForceCoefficient = 0.16f;
    inline constexpr float coastBrakeForce = 0.0f;
    inline constexpr float minimumBrakingSpeed = 0.01f;
    inline constexpr float tireGrip = 2.5f;
    inline constexpr float wheelRadius = 0.48f;
    inline constexpr float wheelWidth = 0.30f;
    inline constexpr float chassisRideHeightOffset = 0.08f;
    inline constexpr float suspensionRestLength = 0.18f;
    inline constexpr float suspensionStiffness = 140.0f;
    inline constexpr float suspensionCompression = 12.0f;
    inline constexpr float suspensionDamping = 15.0f;
    inline constexpr float suspensionMaxTravelCm = 16.0f;
    inline constexpr float suspensionMaxForce = 16000.0f;
    inline constexpr float wheelRollInfluence = 0.08f;
    inline constexpr float lateralCorrection = 8.0f;

    // Steering and stability controls.
    inline constexpr float maxYawRate = 1.8f;
    inline constexpr float steeringResponseRate = 1.0f;
    inline constexpr float reverseSteeringThreshold = -0.1f;
    inline constexpr float groundedNormalThreshold = 0.2f;
    inline constexpr float directionLengthThreshold = 0.0001f;
    inline constexpr float rollPitchDamping = 2.0f;
    inline constexpr float maxRollPitchRate = 2.5f;
    inline constexpr float uprightAlignmentThreshold = 0.8660254f;
    inline constexpr float uprightCorrectionTorque = 35.0f;

    // Bullet rigid-body contact and damping settings.
    inline constexpr float bodyFriction = 0.8f;
    inline constexpr float bodyRestitution = 0.0f;
    inline constexpr float bodyLinearDamping = 0.05f;
    inline constexpr float bodyAngularDamping = 0.85f;
}

#endif
