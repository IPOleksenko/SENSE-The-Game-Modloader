#pragma once

#include "types.hpp"
#include <vector>

namespace sense {

class CameraController {
public:
    static CameraController& instance();

    // Zoom & Scale (Centered on screen)
    void setZoom(float zoom);
    float getZoom() const;

    // Position & Pan Offset
    void setOffset(float x, float y);
    PointF getOffset() const;

    // Rotation Angle (Degrees)
    void setAngle(float degrees);
    float getAngle() const;

    // Pseudo-3D Perspective Tilt (Pitch & Yaw)
    // tiltX: Yaw (horizontal tilt, -1.0 to +1.0)
    // tiltY: Pitch (vertical tilt / road vanishing point, -1.0 to +1.0)
    void setPerspectiveTilt(float tiltX, float tiltY);
    PointF getPerspectiveTilt() const;

    // Viewport Clipping
    void setViewport(const Rect& rect);
    void resetViewport();
    bool hasCustomViewport() const;
    Rect getViewport() const;

    // Flip & Mirror
    void setFlipped(bool horizontal, bool vertical);
    bool isFlippedH() const;
    bool isFlippedV() const;

    // Screen Shake & Trauma
    void shake(float durationSeconds, float intensity);
    void stopShake();

    // Full-screen Color Filter / Tint
    void setColorFilter(const Color& color, ColorBlendMode blendMode = ColorBlendMode::Blend);
    void clearColorFilter();
    bool hasColorFilter() const;

    // Screen Flash
    void flash(const Color& color, float durationSeconds);

    // Preset Visual Styles
    void presetNightVision();
    void presetSepia();
    void presetCyberpunk();
    void presetDamageVignette(float intensity = 0.5f);

    // Preset 3D Perspectives
    void preset3DHighway();
    void presetBirdEye();
    void presetIsometric();

    // Reset everything to default camera
    void reset();

    // Update animation timers (shake, flash)
    void update(float deltaTime);

    // Render pipeline hooks
    void beginScene(SDL_Renderer* renderer, int width = 1280, int height = 720);
    void presentScene(SDL_Renderer* renderer, int screenW = 1280, int screenH = 720);
    void renderPostProcessing(SDL_Renderer* renderer, int screenW, int screenH);

    // Legacy bridge methods
    void applyToRenderer(SDL_Renderer* renderer, int screenW, int screenH) { beginScene(renderer, screenW, screenH); }
    void restoreRenderer(SDL_Renderer* renderer, int screenW, int screenH) { presentScene(renderer, screenW, screenH); }

    bool hasActiveTransform() const;
    const CameraTransform& getTransform() const { return m_transform; }
    bool isTargetActive() const { return m_isTargetActive; }

private:
    CameraController();
    ~CameraController();

    CameraTransform m_transform;
    bool m_customViewport = false;
    Rect m_viewport = { 0, 0, 0, 0 };
    float m_shakeTimeRemaining = 0.0f;
    float m_shakeIntensity = 0.0f;

    SDL_Texture* m_sceneTarget = nullptr;
    int m_targetWidth = 0;
    int m_targetHeight = 0;
    bool m_isTargetActive = false;
    void* m_lastRenderer = nullptr;
};

// Convenience namespace functions
namespace camera {
    inline void setZoom(float zoom) { CameraController::instance().setZoom(zoom); }
    inline float getZoom() { return CameraController::instance().getZoom(); }

    inline void setOffset(float x, float y) { CameraController::instance().setOffset(x, y); }
    inline PointF getOffset() { return CameraController::instance().getOffset(); }

    inline void setAngle(float degrees) { CameraController::instance().setAngle(degrees); }
    inline float getAngle() { return CameraController::instance().getAngle(); }

    inline void setPerspectiveTilt(float tiltX, float tiltY) { CameraController::instance().setPerspectiveTilt(tiltX, tiltY); }
    inline PointF getPerspectiveTilt() { return CameraController::instance().getPerspectiveTilt(); }

    inline void setViewport(const Rect& rect) { CameraController::instance().setViewport(rect); }
    inline void resetViewport() { CameraController::instance().resetViewport(); }

    inline void setFlipped(bool h, bool v) { CameraController::instance().setFlipped(h, v); }
    inline void shake(float duration, float intensity) { CameraController::instance().shake(duration, intensity); }

    inline void setColorFilter(const Color& color, ColorBlendMode mode = ColorBlendMode::Blend) {
        CameraController::instance().setColorFilter(color, mode);
    }
    inline void clearColorFilter() { CameraController::instance().clearColorFilter(); }
    inline void flash(const Color& color, float duration) { CameraController::instance().flash(color, duration); }

    inline void presetNightVision() { CameraController::instance().presetNightVision(); }
    inline void presetSepia() { CameraController::instance().presetSepia(); }
    inline void presetCyberpunk() { CameraController::instance().presetCyberpunk(); }
    inline void preset3DHighway() { CameraController::instance().preset3DHighway(); }
    inline void presetBirdEye() { CameraController::instance().presetBirdEye(); }
    inline void presetIsometric() { CameraController::instance().presetIsometric(); }
    inline void reset() { CameraController::instance().reset(); }
}

} // namespace sense

