#include "SoundManager.h"

#include "../Car/CarConfig.h"

#include <algorithm>
#include <array>
#include <cmath>

SoundManager::SoundManager()
    : main(ASSET_RESOURCES_PATH "Main.wav"),
      mid(ASSET_RESOURCES_PATH "Mid.wav"),
      midHigh(ASSET_RESOURCES_PATH "Mid-High.wav"),
      midMidHigh(ASSET_RESOURCES_PATH "Mid-Mid-High.wav"),
      high(ASSET_RESOURCES_PATH "High.wav"),
      highHigh(ASSET_RESOURCES_PATH "High-High.wav"),
      shift(ASSET_RESOURCES_PATH "Shift.wav") {
    main.setLoop(true);
    mid.setLoop(true);
    midHigh.setLoop(true);
    midMidHigh.setLoop(true);
    high.setLoop(true);
    highHigh.setLoop(true);
    main.setVolume(75.0f);
    mid.setVolume(0.0f);
    midHigh.setVolume(0.0f);
    midMidHigh.setVolume(0.0f);
    highHigh.setVolume(0.0f);
    high.setVolume(0.0f);
    shift.setVolume(0.0f);
}

void SoundManager::start() {
    previousGear = 0;
    main.play();
    mid.play();
    midHigh.play();
    midMidHigh.play();
    high.play();
    highHigh.play();
}

void SoundManager::stop() {
    main.stop();
    mid.stop();
    midHigh.stop();
    midMidHigh.stop();
    high.stop();
    highHigh.stop();
    shift.stop();
    previousGear = 0;
}

void SoundManager::update(const float rpm, const float speedKmh, const int gear) {
    constexpr float maxRPM = 15000.0f;
    constexpr float maxSpeedKmh = 300.0f;
    constexpr float basePitch = 0.85f;
    constexpr float rpmPitchRange = 0.3f;
    constexpr float speedPitchRange = 0.65f;
    constexpr float gearPitchRange = 0.08f;
    constexpr float mainVolume = 75.0f;
    constexpr float intenseDownshiftRPMIncrease = 2000.0f;
    constexpr float minimumDownshiftSpeedKmh = 30.0f;
    constexpr float halfPi = 1.57079632679f;
    constexpr float maxVolume = 100.0f;
    constexpr std::array<float, 5> layerVolumes{90.0f, 90.0f, 90.0f, 90.0f, 90.0f};
    constexpr std::array<float, 4> fadeStarts{4000.0f, 6500.0f, 9000.0f, 11500.0f};
    constexpr std::array<float, 4> fadeEnds{7000.0f, 9500.0f, 12000.0f, 15000.0f};

    const float boundedRPM = std::clamp(rpm, 0.0f, maxRPM);
    const int boundedGear =
        std::clamp(gear, 1, static_cast<int>(car_config::gearRatios.size()));
    if (previousGear > 0 && boundedGear < previousGear &&
        speedKmh >= minimumDownshiftSpeedKmh) {
        const float rpmIncrease = boundedRPM *
            (car_config::gearRatios[boundedGear - 1] /
                 car_config::gearRatios[previousGear - 1] -
             1.0f);
        if (rpmIncrease >= intenseDownshiftRPMIncrease) {
            const float shiftVolume = std::clamp(
                30.0f + (rpmIncrease - intenseDownshiftRPMIncrease) / 50.0f,
                30.0f, maxVolume);
            shift.stop();
            shift.setVolume(shiftVolume);
            shift.play();
        }
    }
    previousGear = boundedGear;

    const std::array<float, 5> fadeVolumes = [&] {
        std::array<float, 5> volumes{};
        for (size_t i = 0; i < fadeStarts.size(); ++i) {
            if (boundedRPM < fadeStarts[i]) {
                volumes[i] = layerVolumes[i];
                return volumes;
            }
            if (boundedRPM <= fadeEnds[i]) {
                const float progress = (boundedRPM - fadeStarts[i]) /
                                       (fadeEnds[i] - fadeStarts[i]);
                const float smoothProgress =
                    progress * progress * (3.0f - 2.0f * progress);
                volumes[i] = layerVolumes[i] * std::cos(smoothProgress * halfPi);
                volumes[i + 1] =
                    layerVolumes[i + 1] * std::sin(smoothProgress * halfPi);
                return volumes;
            }
        }
        volumes.back() = layerVolumes.back();
        return volumes;
    }();

    const float rpmPitch = boundedRPM / maxRPM;
    const float speedPitch = std::clamp(speedKmh / maxSpeedKmh, 0.0f, 1.0f);
    const float gearPitch = static_cast<float>(boundedGear - 1) /
                            static_cast<float>(car_config::gearRatios.size() - 1);
    const float enginePitch = basePitch + rpmPitchRange * rpmPitch +
                              speedPitchRange * speedPitch +
                              gearPitchRange * gearPitch;
    main.setVolume(mainVolume);
    main.setPitch(enginePitch);
    mid.setVolume(fadeVolumes[0]);
    mid.setPitch(enginePitch);
    midHigh.setVolume(fadeVolumes[1]);
    midHigh.setPitch(enginePitch);
    midMidHigh.setVolume(fadeVolumes[2]);
    midMidHigh.setPitch(enginePitch);
    high.setVolume(fadeVolumes[3]);
    high.setPitch(enginePitch);
    highHigh.setVolume(fadeVolumes[4]);
    highHigh.setPitch(enginePitch);
}
