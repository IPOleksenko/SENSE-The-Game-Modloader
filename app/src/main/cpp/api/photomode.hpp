#pragma once

#include "types.hpp"
#include <string>

namespace sense {

class PhotoModeController {
public:
    static PhotoModeController& instance();

    void toggle();
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    bool captureScreenshot(SDL_Renderer* renderer, const std::string& customPath = "");

    void setGridVisible(bool visible) { m_showGrid = visible; }
    bool isGridVisible() const { return m_showGrid; }

    void update(float deltaTime);
    bool handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    PhotoModeController();

    bool m_enabled = false;
    bool m_showGrid = true;
    bool m_showHud = true;
    bool m_savedMovingState = false;

    float m_camOffsetX = 0.0f;
    float m_camOffsetY = 0.0f;
    float m_zoom = 1.0f;
    float m_roll = 0.0f;
};

namespace photomode {
    inline void toggle() { PhotoModeController::instance().toggle(); }
    inline bool isEnabled() { return PhotoModeController::instance().isEnabled(); }
    inline bool capture(SDL_Renderer* r, const std::string& path = "") { return PhotoModeController::instance().captureScreenshot(r, path); }
}

} // namespace sense

