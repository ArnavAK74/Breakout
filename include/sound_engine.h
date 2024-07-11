#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <string>
#include <unordered_map>

class SoundEngine {
public:
    SoundEngine();
    ~SoundEngine();
    
    void playBackgroundMusic(const char* filename);
    void stopBackgroundMusic();
    void playSoundEffect(const char* filename, std::string name);
    void setBackgroundMusicVolume(float volume);
    void setSoundEffectVolume(std::string name, float volume);

private:
    ALCdevice* device;
    ALCcontext* context;
    ALuint backgroundMusicSource;
    std::unordered_map<std::string, ALuint> soundEffects;    
    
    ALuint loadSound(const char* filename);
    void cleanupFinishedSources();
};

#endif
