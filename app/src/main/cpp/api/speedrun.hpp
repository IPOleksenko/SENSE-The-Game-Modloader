#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace sense {

struct SplitSegment {
    std::string name;
    float targetY = 0.0f;
    double pbTime = 0.0;     // Personal Best split time in seconds
    double currentTime = 0.0;// Run split time in seconds
    bool completed = false;
};

class SpeedrunController {
public:
    static SpeedrunController& instance();

    void start();
    void pause();
    void resume();
    void reset();
    bool isRunning() const { return m_running; }
    bool isPaused() const { return m_paused; }
    double getElapsedTime() const;

    void addSplit(const std::string& name, float targetY, double pbTime = 0.0);
    const std::vector<SplitSegment>& getSplits() const { return m_splits; }
    void clearSplits();

    void setOverlayVisible(bool visible) { m_overlayVisible = visible; }
    bool isOverlayVisible() const { return m_overlayVisible; }

    void savePBToFile(const std::string& filepath);
    void loadPBFromFile(const std::string& filepath);

    void update(float deltaTime);
    void renderOverlay(SDL_Renderer* renderer, int screenW, int screenH);

    static std::string formatTime(double seconds);

private:
    SpeedrunController();

    bool m_running = false;
    bool m_paused = false;
    bool m_overlayVisible = false;
    double m_elapsedTime = 0.0;
    std::vector<SplitSegment> m_splits;
    size_t m_currentSplitIndex = 0;
};

namespace speedrun {
    inline void start() { SpeedrunController::instance().start(); }
    inline void pause() { SpeedrunController::instance().pause(); }
    inline void resume() { SpeedrunController::instance().resume(); }
    inline void reset() { SpeedrunController::instance().reset(); }
    inline bool isRunning() { return SpeedrunController::instance().isRunning(); }
    inline double getElapsedTime() { return SpeedrunController::instance().getElapsedTime(); }
    inline void addSplit(const std::string& name, float targetY, double pbTime = 0.0) { SpeedrunController::instance().addSplit(name, targetY, pbTime); }
    inline void setOverlayVisible(bool v) { SpeedrunController::instance().setOverlayVisible(v); }
    inline bool isOverlayVisible() { return SpeedrunController::instance().isOverlayVisible(); }
}

} // namespace sense

