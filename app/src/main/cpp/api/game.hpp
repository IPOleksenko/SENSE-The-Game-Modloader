#pragma once

#include "types.hpp"

namespace sense {

class GameController {
public:
    static GameController& instance();

    // Time & Speed Controls (Speedhack & Slow-Mo)
    float getTimeScale() const;
    void setTimeScale(float scale);

    // Pause Control
    bool isPaused() const;
    void setPaused(bool paused);
    void togglePause();

    // Performance Metrics
    float getFPS() const;
    float getDeltaTime() const;
    uint32_t getFrameCount() const;

    // Window & Display
    Point getWindowSize() const;
    void setWindowSize(int width, int height);
    bool isFullscreen() const;
    void setFullscreen(bool fullscreen);

    // Endless Mode
    bool isEndlessMode() const;
    void setEndlessMode(bool endless);
    void setEndlessModeDirect(bool endless);
    void toggleEndlessMode();

    // Game Control Actions
    void restart();
    void killAndReload();
    void quit();

    // Base Memory Address
    uintptr_t getBaseAddress() const;

    // Internal engine tick
    void update(float deltaTime);
    void registerFrame();

private:
    GameController();
    float m_timeScale = 1.0f;
    bool m_paused = false;
    bool m_endlessMode = false;
    float m_fps = 60.0f;
    float m_deltaTime = 0.016f;
    uint32_t m_frameCount = 0;
    Point m_windowSize = { 1280, 720 };
    bool m_fullscreen = false;
    uintptr_t m_baseAddress = 0;
};

// Convenience namespace functions
namespace game {
    inline float getTimeScale() { return GameController::instance().getTimeScale(); }
    inline void setTimeScale(float s) { GameController::instance().setTimeScale(s); }

    inline bool isPaused() { return GameController::instance().isPaused(); }
    inline void setPaused(bool p) { GameController::instance().setPaused(p); }
    inline void togglePause() { GameController::instance().togglePause(); }

    inline float getFPS() { return GameController::instance().getFPS(); }
    inline float getDeltaTime() { return GameController::instance().getDeltaTime(); }
    inline uint32_t getFrameCount() { return GameController::instance().getFrameCount(); }

    inline Point getWindowSize() { return GameController::instance().getWindowSize(); }
    inline void setWindowSize(int w, int h) { GameController::instance().setWindowSize(w, h); }
    inline bool isFullscreen() { return GameController::instance().isFullscreen(); }
    inline void setFullscreen(bool fs) { GameController::instance().setFullscreen(fs); }

    inline bool isEndlessMode() { return GameController::instance().isEndlessMode(); }
    inline void setEndlessMode(bool e) { GameController::instance().setEndlessMode(e); }
    inline void toggleEndlessMode() { GameController::instance().toggleEndlessMode(); }

    inline void restart() { GameController::instance().restart(); }
    inline void killAndReload() { GameController::instance().killAndReload(); }
    inline void quit() { GameController::instance().quit(); }
    inline uintptr_t getBaseAddress() { return GameController::instance().getBaseAddress(); }
}

} // namespace sense

