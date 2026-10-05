#pragma once

#include "types.hpp"
#include <cstddef>
#include <deque>
#include <string>

namespace sense {

struct ProcessMemoryStats {
    size_t workingSetBytes = 0;
    size_t peakWorkingSetBytes = 0;
    size_t privateBytes = 0;
};

class ProfilerController {
public:
    static ProfilerController& instance();

    void recordFrame(float deltaTime);
    void recordDrawCall();
    void resetDrawCalls();

    int getDrawCalls() const { return m_drawCallsLastFrame; }
    float getCurrentFPS() const { return m_currentFPS; }
    float getAverageFPS() const;
    float getOnePercentLowFPS() const;

    ProcessMemoryStats queryMemoryStats() const;

    void toggleOverlay() { m_overlayVisible = !m_overlayVisible; }
    void setOverlayVisible(bool visible) { m_overlayVisible = visible; }
    bool isOverlayVisible() const { return m_overlayVisible; }

    void renderOverlay(SDL_Renderer* renderer, int screenW, int screenH);

private:
    ProfilerController();

    bool m_overlayVisible = false;
    float m_currentFPS = 60.0f;
    int m_drawCallsCurrentFrame = 0;
    int m_drawCallsLastFrame = 0;

    std::deque<float> m_frameTimes; // History of deltaTimes (up to 120 frames)
};

namespace profiler {
    inline void toggleOverlay() { ProfilerController::instance().toggleOverlay(); }
    inline void setOverlayVisible(bool v) { ProfilerController::instance().setOverlayVisible(v); }
    inline bool isOverlayVisible() { return ProfilerController::instance().isOverlayVisible(); }
    inline float getFPS() { return ProfilerController::instance().getCurrentFPS(); }
    inline ProcessMemoryStats getMemoryStats() { return ProfilerController::instance().queryMemoryStats(); }
}

} // namespace sense

