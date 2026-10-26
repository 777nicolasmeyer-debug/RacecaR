#ifndef RACECAR_SOUND_MANAGER_H
#define RACECAR_SOUND_MANAGER_H

#include "Sound.h"

class SoundManager {
public:
    SoundManager();
    void start();
    void stop();
    void update(float rpm, float speedKmh, int gear);

private:
    Sound main;
    Sound mid;
    Sound midHigh;
    Sound midMidHigh;
    Sound high;
    Sound highHigh;
    Sound shift;
    int previousGear = 0;
};

#endif
