#ifndef RACECAR_CAR_CONFIG_H
#define RACECAR_CAR_CONFIG_H

#include <array>

#include <glm/glm.hpp>

namespace car_config {
    inline const glm::vec3 cockpitOffset{0.0f, 0.50f, 0.057f};
    inline const glm::vec3 chassisOffset{0.0f, -0.3f, 0.0f};
    inline const glm::vec3 chassisHalfExtents{0.72f, 0.22f, 1.55f};

    inline constexpr float idleRPM = 1500.0f;
    inline constexpr float redlineRPM = 15000.0f;
    inline constexpr float coastDrag = 0.12f;
    inline constexpr std::array<float, 8> gearMaxSpeeds{
        15.0f, 26.0f, 33.0f, 41.0f, 50.0f, 70.0f, 81.0f, 93.0f
    };
    inline constexpr std::array<float, 8> gearAcceleration{
        3.0f, 2.78f, 2.64f, 2.54f, 2.47f, 1.99f, 1.80f, 1.55f
    };
    inline constexpr std::array<float, 8> gearRpmMultiplier{
        1.35f, 1.2f, 1.1f, 1.0f, 0.95f, 0.9f, 0.85f, 0.8f
    };
    inline constexpr float downforceCoefficient = 0.18f;
    inline constexpr float tireGrip = 1.4f;
    inline constexpr float maxYawRate = 1.4f;
    inline constexpr float brakeForceCoefficient = 0.30f;
}

#endif
