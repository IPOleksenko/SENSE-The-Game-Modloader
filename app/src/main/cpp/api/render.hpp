#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace sense {

struct ToastNotification {
    std::string text;
    float durationSeconds = 3.0f;
    float timeRemaining = 3.0f;
    Color color = Color::White();
};

class RenderController {
public:
    static RenderController& instance();

    // 2D Primitives
    void drawRect(SDL_Renderer* renderer, const Rect& rect, const Color& color);
    void fillRect(SDL_Renderer* renderer, const Rect& rect, const Color& color);
    void drawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, const Color& color);
    void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, const Color& color);
    void fillCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, const Color& color);

    void drawGradientV(SDL_Renderer* renderer, const Rect& rect, const Color& topColor, const Color& bottomColor);
    void drawGradientH(SDL_Renderer* renderer, const Rect& rect, const Color& leftColor, const Color& rightColor);

    // Native Game TTF Font Engine (Powered by game's SDL2_ttf and authentic font)
    void drawText(
        SDL_Renderer* renderer,
        const std::string& text,
        int x,
        int y,
        const Color& color = Color::White(),
        float scale = 1.0f,
        bool shadow = true
    );

    Point getTextSize(const std::string& text, float scale = 1.0f) const;
    void clearFontCache();

    // Perspective / 3D Textured Quad Geometry (SDL_RenderGeometry)
    void drawTrapezoid(
        SDL_Renderer* renderer,
        SDL_Texture* texture,
        const PointF& topLeft,
        const PointF& topRight,
        const PointF& bottomRight,
        const PointF& bottomLeft,
        const Color& color = Color::White()
    );

    // 2D/3D Hardware-accelerated Triangle Geometry
    void drawTriangle(
        SDL_Renderer* renderer,
        const PointF& p1,
        const PointF& p2,
        const PointF& p3,
        const Color& color
    );

    // On-screen Toast Notifications
    void showNotification(const std::string& text, float durationSeconds = 3.0f, const Color& color = Color::Green());
    void updateNotifications(float deltaTime);
    void renderNotifications(SDL_Renderer* renderer, int screenW, int screenH);

private:
    RenderController() = default;
    std::vector<ToastNotification> m_toasts;
};

// Convenience namespace functions
namespace render {
    inline void drawRect(SDL_Renderer* r, const Rect& rect, const Color& c) { RenderController::instance().drawRect(r, rect, c); }
    inline void fillRect(SDL_Renderer* r, const Rect& rect, const Color& c) { RenderController::instance().fillRect(r, rect, c); }
    inline void drawLine(SDL_Renderer* r, int x1, int y1, int x2, int y2, const Color& c) { RenderController::instance().drawLine(r, x1, y1, x2, y2, c); }
    inline void drawCircle(SDL_Renderer* r, int cx, int cy, int rad, const Color& c) { RenderController::instance().drawCircle(r, cx, cy, rad, c); }
    inline void fillCircle(SDL_Renderer* r, int cx, int cy, int rad, const Color& c) { RenderController::instance().fillCircle(r, cx, cy, rad, c); }

    inline void drawText(SDL_Renderer* r, const std::string& txt, int x, int y, const Color& c = Color::White(), float scale = 1.0f, bool shadow = true) {
        RenderController::instance().drawText(r, txt, x, y, c, scale, shadow);
    }
    inline Point getTextSize(const std::string& txt, float scale = 1.0f) {
        return RenderController::instance().getTextSize(txt, scale);
    }

    inline void showNotification(const std::string& text, float duration = 3.0f, const Color& c = Color::Green()) {
        RenderController::instance().showNotification(text, duration, c);
    }
    inline void showNotification(const std::string& text, int durationMs, const Color& c = Color::Green()) {
        RenderController::instance().showNotification(text, static_cast<float>(durationMs) / 1000.0f, c);
    }
}

} // namespace sense

