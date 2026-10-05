#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <memory>

namespace sense {

struct ReplaySample {
    float time = 0.0f;
    int posY = 0;
    float speed = 0.0f;
    int currentFrame = 0;
    PlayerMove move = PlayerMove::Undefined;
    float balance = 0.0f;
};

struct ReplayRun {
    std::string name;
    float totalTime = 0.0f;
    int finalPosY = 0;
    std::vector<ReplaySample> samples;
    bool isValid() const { return !samples.empty(); }
};

class ReplayController {
public:
    static ReplayController& instance();

    // 1. Recording
    void startRecording(const std::string& runName = "Run");
    void stopRecording();
    bool isRecording() const;
    const ReplayRun& getCurrentRun() const;

    // 2. Playback Mode
    void startPlayback(const ReplayRun& run);
    void stopPlayback();
    bool isPlaybackActive() const;
    float getPlaybackTime() const;

    // 3. Ghost Player Racing
    void setGhostEnabled(bool enabled);
    bool isGhostEnabled() const;
    void setGhostRun(const ReplayRun& run);
    const ReplayRun& getGhostRun() const;
    void setGhostColor(const Color& color);
    Color getGhostColor() const;
    void setGhostAlpha(uint8_t alpha);
    uint8_t getGhostAlpha() const;

    // 4. File I/O
    bool saveReplayToFile(const ReplayRun& run, const std::string& filePath);
    bool loadReplayFromFile(const std::string& filePath, ReplayRun& outRun);

    // Engine Lifecycle
    void update(float deltaTime);
    void renderGhost(SDL_Renderer* renderer, int screenW, int screenH);

private:
    ReplayController();

    bool m_isRecording = false;
    float m_recordTimer = 0.0f;
    ReplayRun m_currentRecording;

    bool m_isPlayback = false;
    float m_playbackTimer = 0.0f;
    ReplayRun m_playbackRun;

    bool m_ghostEnabled = false;
    ReplayRun m_ghostRun;
    Color m_ghostColor = Color::Cyan();
    uint8_t m_ghostAlpha = 140;

    float m_liveGameTimer = 0.0f;
};

// Convenience namespace functions
namespace replay {
    inline void startRecording(const std::string& name = "Run") { ReplayController::instance().startRecording(name); }
    inline void stopRecording() { ReplayController::instance().stopRecording(); }
    inline bool isRecording() { return ReplayController::instance().isRecording(); }

    inline void setGhostEnabled(bool en) { ReplayController::instance().setGhostEnabled(en); }
    inline bool isGhostEnabled() { return ReplayController::instance().isGhostEnabled(); }
    inline void setGhostRun(const ReplayRun& run) { ReplayController::instance().setGhostRun(run); }
    inline void setGhostColor(const Color& c) { ReplayController::instance().setGhostColor(c); }

    inline bool saveToFile(const ReplayRun& r, const std::string& path) { return ReplayController::instance().saveReplayToFile(r, path); }
    inline bool loadFromFile(const std::string& path, ReplayRun& r) { return ReplayController::instance().loadReplayFromFile(path, r); }
}

} // namespace sense

