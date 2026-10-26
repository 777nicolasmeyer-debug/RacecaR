#ifndef RACECAR_SOUND_H
#define RACECAR_SOUND_H

#include <SFML/Audio.hpp>

class Sound {
public:
    Sound(const char *path);

    void play();
    void stop();
    void setPitch(float pitch);
    void setVolume(float volume);
    void setLoop(bool loop);

private:
    sf::SoundBuffer buffer;
    sf::Sound sound;
};

class Music {
public:
    sf::Music music;
};

#endif
