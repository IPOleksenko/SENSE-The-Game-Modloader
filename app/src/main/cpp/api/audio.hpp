#pragma once

#include "types.hpp"
#include <string>

namespace sense {

class AudioController {
public:
    static AudioController& instance();

    // Volume Control (0 to 128)
    void setMusicVolume(int volume);
    int getMusicVolume() const;

    void setSfxVolume(int volume);
    int getSfxVolume() const;

    // Music playback
    void pauseMusic();
    void resumeMusic();
    void stopMusic();

    // Custom sound playback from file
    bool playSoundFile(const std::string& filePath, int volume = 128);

private:
    AudioController();
    int m_musicVolume = 128;
    int m_sfxVolume = 128;
};

// Convenience namespace functions
namespace audio {
    inline void setMusicVolume(int v) { AudioController::instance().setMusicVolume(v); }
    inline int getMusicVolume() { return AudioController::instance().getMusicVolume(); }

    inline void setSfxVolume(int v) { AudioController::instance().setSfxVolume(v); }
    inline int getSfxVolume() { return AudioController::instance().getSfxVolume(); }

    inline void pauseMusic() { AudioController::instance().pauseMusic(); }
    inline void resumeMusic() { AudioController::instance().resumeMusic(); }
    inline void stopMusic() { AudioController::instance().stopMusic(); }

    inline bool playSound(const std::string& path, int vol = 128) {
        return AudioController::instance().playSoundFile(path, vol);
    }
}

} // namespace sense

