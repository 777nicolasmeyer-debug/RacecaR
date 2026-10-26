#include "Sound.h"

Sound::Sound(const char *path)
    : buffer(path), sound(buffer) {
}

void Sound::play() {
    sound.play();
}

void Sound::stop() {
    sound.stop();
}

void Sound::setPitch(const float pitch) {
    sound.setPitch(pitch);
}

void Sound::setVolume(const float volume) {
    sound.setVolume(volume);
}

void Sound::setLoop(const bool loop) {
    sound.setLooping(loop);
}
