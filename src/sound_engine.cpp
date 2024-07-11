#include "sound_engine.h"
#include <iostream>

SoundEngine::SoundEngine() 
{
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open OpenAL device" << std::endl;
        return;
    }

    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create OpenAL context" << std::endl;
        alcCloseDevice(device);
        return;
    }

    alcMakeContextCurrent(context);
    alGenSources(1, &backgroundMusicSource);
}

SoundEngine::~SoundEngine() 
{
    alDeleteSources(1, &backgroundMusicSource);
    for (auto& pair : soundEffects) {
        alDeleteSources(1, &pair.second);
    }
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void SoundEngine::playBackgroundMusic(const char* filename) 
{
    ALuint buffer = loadSound(filename);
    if (buffer == 0) {
        return;
    }

    alSourcei(backgroundMusicSource, AL_BUFFER, buffer);
    alSourcei(backgroundMusicSource, AL_LOOPING, AL_TRUE);
    alSourcePlay(backgroundMusicSource);
}

void SoundEngine::setBackgroundMusicVolume(float volume)
{
    alSourcef(backgroundMusicSource, AL_GAIN, volume);
}

void SoundEngine::stopBackgroundMusic()
{
    alSourceStop(backgroundMusicSource);
}

void SoundEngine::playSoundEffect(const char* filename, std::string name) 
{
    cleanupFinishedSources();

    ALuint buffer = loadSound(filename);
    if (buffer == 0) {
        return;
    }

    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    soundEffects[name] = source;
}

void SoundEngine::setSoundEffectVolume(std::string name, float volume)
{
    auto it = soundEffects.find(name);
    if (it != soundEffects.end()) {
        alSourcef(it->second, AL_GAIN, volume);
    }
}

void SoundEngine::cleanupFinishedSources() 
{
    std::vector<std::string> finishedEffects;
    for (auto& pair : soundEffects) {
        ALint state;
        alGetSourcei(pair.second, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            alDeleteSources(1, &pair.second);
            finishedEffects.push_back(pair.first);
        }
    }
    for (const auto& effect : finishedEffects) {
        soundEffects.erase(effect);
    }
}

ALuint SoundEngine::loadSound(const char* filename) 
{
    SF_INFO sfInfo;
    SNDFILE* sndFile = sf_open(filename, SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "Failed to open audio file: " << filename << std::endl;
        return 0;
    }

    short* bufferData = new short[sfInfo.frames * sfInfo.channels];
    sf_readf_short(sndFile, bufferData, sfInfo.frames);
    sf_close(sndFile);

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, sfInfo.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
                 bufferData, sfInfo.frames * sfInfo.channels * sizeof(short),
                 sfInfo.samplerate);

    delete[] bufferData;
    return buffer;
}
