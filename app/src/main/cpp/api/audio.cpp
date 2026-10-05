#include "audio.hpp"

namespace sense {

typedef int (*PFN_Mix_VolumeMusic)(int);
typedef int (*PFN_Mix_Volume)(int, int);
typedef void (*PFN_Mix_PauseMusic)();
typedef void (*PFN_Mix_ResumeMusic)();
typedef int (*PFN_Mix_HaltMusic)();
typedef void* (*PFN_Mix_LoadWAV_RW)(void*, int);
typedef int (*PFN_Mix_PlayChannelTimed)(int, void*, int, int);
typedef void* (*PFN_SDL_RWFromFile)(const char*, const char*);

static PFN_Mix_VolumeMusic fn_Mix_VolumeMusic = nullptr;
static PFN_Mix_Volume fn_Mix_Volume = nullptr;
static PFN_Mix_PauseMusic fn_Mix_PauseMusic = nullptr;
static PFN_Mix_ResumeMusic fn_Mix_ResumeMusic = nullptr;
static PFN_Mix_HaltMusic fn_Mix_HaltMusic = nullptr;
static PFN_Mix_LoadWAV_RW fn_Mix_LoadWAV_RW = nullptr;
static PFN_Mix_PlayChannelTimed fn_Mix_PlayChannelTimed = nullptr;
static PFN_SDL_RWFromFile fn_SDL_RWFromFile = nullptr;

static void initAudioSdl() {
    static bool init = false;
    if (init) return;
    init = true;

    HMODULE hMixer = GetModuleHandleA("SDL2_mixer.dll");
    if (hMixer) {
        fn_Mix_VolumeMusic = reinterpret_cast<PFN_Mix_VolumeMusic>(GetProcAddress(hMixer, "Mix_VolumeMusic"));
        fn_Mix_Volume = reinterpret_cast<PFN_Mix_Volume>(GetProcAddress(hMixer, "Mix_Volume"));
        fn_Mix_PauseMusic = reinterpret_cast<PFN_Mix_PauseMusic>(GetProcAddress(hMixer, "Mix_PauseMusic"));
        fn_Mix_ResumeMusic = reinterpret_cast<PFN_Mix_ResumeMusic>(GetProcAddress(hMixer, "Mix_ResumeMusic"));
        fn_Mix_HaltMusic = reinterpret_cast<PFN_Mix_HaltMusic>(GetProcAddress(hMixer, "Mix_HaltMusic"));
        fn_Mix_LoadWAV_RW = reinterpret_cast<PFN_Mix_LoadWAV_RW>(GetProcAddress(hMixer, "Mix_LoadWAV_RW"));
        fn_Mix_PlayChannelTimed = reinterpret_cast<PFN_Mix_PlayChannelTimed>(GetProcAddress(hMixer, "Mix_PlayChannelTimed"));
    }

    HMODULE hSdl = GetModuleHandleA("SDL2.dll");
    if (hSdl) {
        fn_SDL_RWFromFile = reinterpret_cast<PFN_SDL_RWFromFile>(GetProcAddress(hSdl, "SDL_RWFromFile"));
    }
}

AudioController::AudioController() {
    initAudioSdl();
}

AudioController& AudioController::instance() {
    static AudioController inst;
    return inst;
}

void AudioController::setMusicVolume(int volume) {
    m_musicVolume = std::clamp(volume, 0, 128);
    initAudioSdl();
    if (fn_Mix_VolumeMusic) {
        fn_Mix_VolumeMusic(m_musicVolume);
    }
}

int AudioController::getMusicVolume() const {
    return m_musicVolume;
}

void AudioController::setSfxVolume(int volume) {
    m_sfxVolume = std::clamp(volume, 0, 128);
    initAudioSdl();
    if (fn_Mix_Volume) {
        fn_Mix_Volume(-1, m_sfxVolume);
    }
}

int AudioController::getSfxVolume() const {
    return m_sfxVolume;
}

void AudioController::pauseMusic() {
    initAudioSdl();
    if (fn_Mix_PauseMusic) fn_Mix_PauseMusic();
}

void AudioController::resumeMusic() {
    initAudioSdl();
    if (fn_Mix_ResumeMusic) fn_Mix_ResumeMusic();
}

void AudioController::stopMusic() {
    initAudioSdl();
    if (fn_Mix_HaltMusic) fn_Mix_HaltMusic();
}

bool AudioController::playSoundFile(const std::string& filePath, int volume) {
    initAudioSdl();
    if (!fn_SDL_RWFromFile || !fn_Mix_LoadWAV_RW || !fn_Mix_PlayChannelTimed) {
        return false;
    }

    void* rw = fn_SDL_RWFromFile(filePath.c_str(), "rb");
    if (!rw) return false;

    void* chunk = fn_Mix_LoadWAV_RW(rw, 1);
    if (!chunk) return false;

    int channel = fn_Mix_PlayChannelTimed(-1, chunk, 0, -1);
    if (channel != -1 && fn_Mix_Volume) {
        fn_Mix_Volume(channel, std::clamp(volume, 0, 128));
    }
    return channel != -1;
}

} // namespace sense

