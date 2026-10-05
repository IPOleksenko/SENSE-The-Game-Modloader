#include "camera.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <vector>

namespace sense {

// Function pointers to SDL2 runtime exports
typedef SDL_Texture* (*PFN_SDL_CreateTexture)(SDL_Renderer*, uint32_t, int, int, int);
typedef void (*PFN_SDL_DestroyTexture)(SDL_Texture*);
typedef int (*PFN_SDL_SetRenderTarget)(SDL_Renderer*, SDL_Texture*);
typedef SDL_Texture* (*PFN_SDL_GetRenderTarget)(SDL_Renderer*);
typedef int (*PFN_SDL_RenderTargetSupported)(SDL_Renderer*);
typedef int (*PFN_SDL_RenderCopyEx)(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*, double, const SDL_Point*, int);
typedef int (*PFN_SDL_RenderGeometry)(SDL_Renderer*, SDL_Texture*, const SDL_Vertex*, int, const int*, int);
typedef int (*PFN_SDL_SetRenderDrawBlendMode)(SDL_Renderer*, int);
typedef int (*PFN_SDL_SetRenderDrawColor)(SDL_Renderer*, uint8_t, uint8_t, uint8_t, uint8_t);
typedef int (*PFN_SDL_RenderFillRect)(SDL_Renderer*, const SDL_Rect*);
typedef int (*PFN_SDL_RenderClear)(SDL_Renderer*);
typedef int (*PFN_SDL_RenderSetClipRect)(SDL_Renderer*, const SDL_Rect*);

static PFN_SDL_CreateTexture fn_SDL_CreateTexture = nullptr;
static PFN_SDL_DestroyTexture fn_SDL_DestroyTexture = nullptr;
static PFN_SDL_SetRenderTarget fn_SDL_SetRenderTarget = nullptr;
static PFN_SDL_GetRenderTarget fn_SDL_GetRenderTarget = nullptr;
static PFN_SDL_RenderTargetSupported fn_SDL_RenderTargetSupported = nullptr;
static PFN_SDL_RenderCopyEx fn_SDL_RenderCopyEx = nullptr;
static PFN_SDL_RenderGeometry fn_SDL_RenderGeometry = nullptr;
static PFN_SDL_SetRenderDrawBlendMode fn_SDL_SetRenderDrawBlendMode = nullptr;
static PFN_SDL_SetRenderDrawColor fn_SDL_SetRenderDrawColor = nullptr;
static PFN_SDL_RenderFillRect fn_SDL_RenderFillRect = nullptr;
static PFN_SDL_RenderClear fn_SDL_RenderClear = nullptr;
static PFN_SDL_RenderSetClipRect fn_SDL_RenderSetClipRect = nullptr;

static void initSdlPointers() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    HMODULE hSdl = GetModuleHandleA("SDL2.dll");
    if (!hSdl) return;

    fn_SDL_CreateTexture = reinterpret_cast<PFN_SDL_CreateTexture>(GetProcAddress(hSdl, "SDL_CreateTexture"));
    fn_SDL_DestroyTexture = reinterpret_cast<PFN_SDL_DestroyTexture>(GetProcAddress(hSdl, "SDL_DestroyTexture"));
    fn_SDL_SetRenderTarget = reinterpret_cast<PFN_SDL_SetRenderTarget>(GetProcAddress(hSdl, "SDL_SetRenderTarget"));
    fn_SDL_GetRenderTarget = reinterpret_cast<PFN_SDL_GetRenderTarget>(GetProcAddress(hSdl, "SDL_GetRenderTarget"));
    fn_SDL_RenderTargetSupported = reinterpret_cast<PFN_SDL_RenderTargetSupported>(GetProcAddress(hSdl, "SDL_RenderTargetSupported"));
    fn_SDL_RenderCopyEx = reinterpret_cast<PFN_SDL_RenderCopyEx>(GetProcAddress(hSdl, "SDL_RenderCopyEx"));
    fn_SDL_RenderGeometry = reinterpret_cast<PFN_SDL_RenderGeometry>(GetProcAddress(hSdl, "SDL_RenderGeometry"));
    fn_SDL_SetRenderDrawBlendMode = reinterpret_cast<PFN_SDL_SetRenderDrawBlendMode>(GetProcAddress(hSdl, "SDL_SetRenderDrawBlendMode"));
    fn_SDL_SetRenderDrawColor = reinterpret_cast<PFN_SDL_SetRenderDrawColor>(GetProcAddress(hSdl, "SDL_SetRenderDrawColor"));
    fn_SDL_RenderFillRect = reinterpret_cast<PFN_SDL_RenderFillRect>(GetProcAddress(hSdl, "SDL_RenderFillRect"));
    fn_SDL_RenderClear = reinterpret_cast<PFN_SDL_RenderClear>(GetProcAddress(hSdl, "SDL_RenderClear"));
    fn_SDL_RenderSetClipRect = reinterpret_cast<PFN_SDL_RenderSetClipRect>(GetProcAddress(hSdl, "SDL_RenderSetClipRect"));
}

CameraController::CameraController() {
    reset();
}

CameraController::~CameraController() {
    if (m_sceneTarget && fn_SDL_DestroyTexture) {
        fn_SDL_DestroyTexture(m_sceneTarget);
        m_sceneTarget = nullptr;
    }
}

CameraController& CameraController::instance() {
    static CameraController cam;
    return cam;
}

void CameraController::setZoom(float zoom) {
    m_transform.zoom = (zoom < 0.05f) ? 0.05f : zoom;
}

float CameraController::getZoom() const {
    return m_transform.zoom;
}

void CameraController::setOffset(float x, float y) {
    m_transform.offsetX = x;
    m_transform.offsetY = y;
}

PointF CameraController::getOffset() const {
    return PointF(m_transform.offsetX, m_transform.offsetY);
}

void CameraController::setAngle(float degrees) {
    m_transform.angle = degrees;
}

float CameraController::getAngle() const {
    return m_transform.angle;
}

void CameraController::setPerspectiveTilt(float tiltX, float tiltY) {
    m_transform.tiltX = tiltX;
    m_transform.tiltY = tiltY;
}

PointF CameraController::getPerspectiveTilt() const {
    return PointF(m_transform.tiltX, m_transform.tiltY);
}

void CameraController::setViewport(const Rect& rect) {
    m_viewport = rect;
    m_customViewport = true;
}

void CameraController::resetViewport() {
    m_customViewport = false;
}

bool CameraController::hasCustomViewport() const {
    return m_customViewport;
}

Rect CameraController::getViewport() const {
    return m_viewport;
}

void CameraController::setFlipped(bool horizontal, bool vertical) {
    m_transform.flipH = horizontal;
    m_transform.flipV = vertical;
}

bool CameraController::isFlippedH() const { return m_transform.flipH; }
bool CameraController::isFlippedV() const { return m_transform.flipV; }

void CameraController::shake(float durationSeconds, float intensity) {
    m_shakeTimeRemaining = durationSeconds;
    m_shakeIntensity = intensity;
}

void CameraController::stopShake() {
    m_shakeTimeRemaining = 0.0f;
    m_transform.currentShakeX = 0.0f;
    m_transform.currentShakeY = 0.0f;
}

void CameraController::setColorFilter(const Color& color, ColorBlendMode blendMode) {
    m_transform.filterEnabled = true;
    m_transform.filterColor = color;
    m_transform.filterBlend = blendMode;
}

void CameraController::clearColorFilter() {
    m_transform.filterEnabled = false;
}

bool CameraController::hasColorFilter() const {
    return m_transform.filterEnabled;
}

void CameraController::flash(const Color& color, float durationSeconds) {
    m_transform.flashTimer = durationSeconds;
    m_transform.flashDuration = durationSeconds;
    m_transform.flashColor = color;
}

void CameraController::presetNightVision() {
    setColorFilter(Color(10, 190, 45, 95), ColorBlendMode::Blend);
}

void CameraController::presetSepia() {
    setColorFilter(Color(112, 66, 20, 100), ColorBlendMode::Blend);
}

void CameraController::presetCyberpunk() {
    setColorFilter(Color(255, 0, 128, 65), ColorBlendMode::Blend);
}

void CameraController::presetDamageVignette(float intensity) {
    uint8_t a = static_cast<uint8_t>(std::clamp(intensity, 0.0f, 1.0f) * 200.0f);
    setColorFilter(Color(255, 0, 0, a), ColorBlendMode::Blend);
}

void CameraController::preset3DHighway() {
    reset();
    setPerspectiveTilt(0.0f, 0.42f);
    setZoom(1.15f);
}

void CameraController::presetBirdEye() {
    reset();
    setPerspectiveTilt(0.0f, -0.35f);
    setZoom(0.90f);
}

void CameraController::presetIsometric() {
    reset();
    setPerspectiveTilt(0.28f, 0.22f);
    setZoom(1.05f);
}

void CameraController::reset() {
    m_transform = CameraTransform();
    m_customViewport = false;
    m_shakeTimeRemaining = 0.0f;
    m_shakeIntensity = 0.0f;
}

void CameraController::update(float deltaTime) {
    // Screen shake update with quadratic decay
    if (m_shakeTimeRemaining > 0.0f) {
        m_shakeTimeRemaining -= deltaTime;
        float factor = (m_shakeTimeRemaining > 0.0f) ? (m_shakeTimeRemaining / 1.0f) : 0.0f;
        if (factor > 1.0f) factor = 1.0f;

        float currentPower = m_shakeIntensity * factor;
        m_transform.currentShakeX = ((rand() % 2001 - 1000) / 1000.0f) * currentPower;
        m_transform.currentShakeY = ((rand() % 2001 - 1000) / 1000.0f) * currentPower;
    } else {
        m_transform.currentShakeX = 0.0f;
        m_transform.currentShakeY = 0.0f;
    }

    // Flash decay
    if (m_transform.flashTimer > 0.0f) {
        m_transform.flashTimer -= deltaTime;
        if (m_transform.flashTimer < 0.0f) {
            m_transform.flashTimer = 0.0f;
        }
    }
}

bool CameraController::hasActiveTransform() const {
    if (std::abs(m_transform.zoom - 1.0f) > 0.005f) return true;
    if (std::abs(m_transform.angle) > 0.05f) return true;
    if (std::abs(m_transform.tiltX) > 0.005f) return true;
    if (std::abs(m_transform.tiltY) > 0.005f) return true;
    if (std::abs(m_transform.offsetX) > 0.5f) return true;
    if (std::abs(m_transform.offsetY) > 0.5f) return true;
    if (m_transform.flipH || m_transform.flipV) return true;
    if (std::abs(m_transform.currentShakeX) > 0.1f || std::abs(m_transform.currentShakeY) > 0.1f) return true;
    if (m_customViewport) return true;
    if (m_transform.filterEnabled || m_transform.flashTimer > 0.0f) return true;
    return false;
}

void CameraController::beginScene(SDL_Renderer* renderer, int width, int height) {
    initSdlPointers();
    if (!renderer || !fn_SDL_SetRenderTarget) return;

    // In normal 2D mode with no camera distortion, render directly to native backbuffer
    // This completely prevents black bars in fullscreen mode!
    if (!hasActiveTransform()) {
        m_isTargetActive = false;
        return;
    }

    if (fn_SDL_RenderTargetSupported && !fn_SDL_RenderTargetSupported(renderer)) {
        m_isTargetActive = false;
        return;
    }

    if (m_lastRenderer != renderer) {
        if (m_sceneTarget && fn_SDL_DestroyTexture) {
            fn_SDL_DestroyTexture(m_sceneTarget);
            m_sceneTarget = nullptr;
        }
        m_lastRenderer = renderer;
    }

    if (!m_sceneTarget || m_targetWidth != width || m_targetHeight != height) {
        if (m_sceneTarget && fn_SDL_DestroyTexture) {
            fn_SDL_DestroyTexture(m_sceneTarget);
            m_sceneTarget = nullptr;
        }

        // Try RGBA8888 (0x16362004) first, then ARGB8888 (0x16161e04)
        if (fn_SDL_CreateTexture) {
            m_sceneTarget = fn_SDL_CreateTexture(renderer, 0x16362004, 2 /* SDL_TEXTUREACCESS_TARGET */, width, height);
            if (!m_sceneTarget) {
                m_sceneTarget = fn_SDL_CreateTexture(renderer, 0x16161e04, 2, width, height);
            }
        }
        m_targetWidth = width;
        m_targetHeight = height;
    }

    if (m_sceneTarget) {
        fn_SDL_SetRenderTarget(renderer, m_sceneTarget);
        m_isTargetActive = true;
    }
}

void CameraController::presentScene(SDL_Renderer* renderer, int screenW, int screenH) {
    initSdlPointers();
    if (!renderer) return;

    if (m_isTargetActive && m_sceneTarget) {
        // 1. Switch target back to main window screen
        fn_SDL_SetRenderTarget(renderer, nullptr);
        m_isTargetActive = false;

        // 2. Clear window backbuffer
        if (fn_SDL_SetRenderDrawColor && fn_SDL_RenderClear) {
            fn_SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            fn_SDL_RenderClear(renderer);
        }

        const int W = (m_targetWidth > 0) ? m_targetWidth : screenW;
        const int H = (m_targetHeight > 0) ? m_targetHeight : screenH;
        const float cx = W * 0.5f;
        const float cy = H * 0.5f;

        // Apply custom viewport clip if enabled
        if (m_customViewport && fn_SDL_RenderSetClipRect) {
            SDL_Rect clip = { m_viewport.x, m_viewport.y, m_viewport.w, m_viewport.h };
            fn_SDL_RenderSetClipRect(renderer, &clip);
        }

        bool hasTilt = (std::abs(m_transform.tiltX) > 0.001f || std::abs(m_transform.tiltY) > 0.001f);

        if (hasTilt && fn_SDL_RenderGeometry) {
            // Render with high-precision 3D perspective geometry mesh
            const int GRID_X = 16;
            const int GRID_Y = 16;
            const int numVerts = (GRID_X + 1) * (GRID_Y + 1);
            const int numTris = GRID_X * GRID_Y * 2;

            std::vector<SDL_Vertex> verts(numVerts);
            std::vector<int> indices(numTris * 3);

            float rad = m_transform.angle * (3.14159265358979323846f / 180.0f);
            float cosA = cosf(rad);
            float sinA = sinf(rad);

            int vIdx = 0;
            for (int j = 0; j <= GRID_Y; ++j) {
                float v = static_cast<float>(j) / GRID_Y;
                float ny = (v - 0.5f) * 2.0f; // -1.0 (top) to +1.0 (bottom)
                float y0 = (v - 0.5f) * H;

                for (int i = 0; i <= GRID_X; ++i) {
                    float u = static_cast<float>(i) / GRID_X;
                    float nx = (u - 0.5f) * 2.0f; // -1.0 (left) to +1.0 (right)
                    float x0 = (u - 0.5f) * W;

                    // 3D Perspective Depth formula:
                    // Positive tiltY tilts top into distance (horizon), bottom widens towards player
                    float depth = 1.0f - (ny * m_transform.tiltY) + (nx * m_transform.tiltX);
                    if (depth < 0.15f) depth = 0.15f;

                    float px = (x0 / depth) * m_transform.zoom;
                    float py = (y0 / depth) * m_transform.zoom;

                    // 2D Rotation around center
                    float rx = px * cosA - py * sinA;
                    float ry = px * sinA + py * cosA;

                    // Pan offset and screen shake
                    float finalX = cx + rx + m_transform.offsetX + m_transform.currentShakeX;
                    float finalY = cy + ry + m_transform.offsetY + m_transform.currentShakeY;

                    verts[vIdx].position.x = finalX;
                    verts[vIdx].position.y = finalY;
                    verts[vIdx].color.r = 255;
                    verts[vIdx].color.g = 255;
                    verts[vIdx].color.b = 255;
                    verts[vIdx].color.a = 255;
                    verts[vIdx].tex_coord.x = m_transform.flipH ? (1.0f - u) : u;
                    verts[vIdx].tex_coord.y = m_transform.flipV ? (1.0f - v) : v;
                    vIdx++;
                }
            }

            int iIdx = 0;
            for (int j = 0; j < GRID_Y; ++j) {
                for (int i = 0; i < GRID_X; ++i) {
                    int p0 = j * (GRID_X + 1) + i;
                    int p1 = p0 + 1;
                    int p2 = (j + 1) * (GRID_X + 1) + i;
                    int p3 = p2 + 1;

                    // Triangle 1
                    indices[iIdx++] = p0;
                    indices[iIdx++] = p1;
                    indices[iIdx++] = p2;

                    // Triangle 2
                    indices[iIdx++] = p1;
                    indices[iIdx++] = p3;
                    indices[iIdx++] = p2;
                }
            }

            fn_SDL_RenderGeometry(renderer, m_sceneTarget, verts.data(), numVerts, indices.data(), static_cast<int>(indices.size()));
        } else if (fn_SDL_RenderCopyEx) {
            // Render with centered zoom, rotation, pan, flip
            SDL_Rect srcRect = { 0, 0, W, H };

            float scaledW = W * m_transform.zoom;
            float scaledH = H * m_transform.zoom;

            int dstX = static_cast<int>(cx - (scaledW * 0.5f) + m_transform.offsetX + m_transform.currentShakeX);
            int dstY = static_cast<int>(cy - (scaledH * 0.5f) + m_transform.offsetY + m_transform.currentShakeY);

            SDL_Rect dstRect = {
                dstX,
                dstY,
                static_cast<int>(scaledW),
                static_cast<int>(scaledH)
            };

            SDL_Point center = {
                static_cast<int>(scaledW * 0.5f),
                static_cast<int>(scaledH * 0.5f)
            };

            int flipFlags = 0;
            if (m_transform.flipH) flipFlags |= 1; // SDL_FLIP_HORIZONTAL
            if (m_transform.flipV) flipFlags |= 2; // SDL_FLIP_VERTICAL

            fn_SDL_RenderCopyEx(renderer, m_sceneTarget, &srcRect, &dstRect, m_transform.angle, &center, flipFlags);
        }

        // Reset clip rect if was set
        if (m_customViewport && fn_SDL_RenderSetClipRect) {
            fn_SDL_RenderSetClipRect(renderer, nullptr);
        }

        // Apply visual post-processing (tint, flash)
        renderPostProcessing(renderer, W, H);
    } else {
        if (m_transform.filterEnabled || m_transform.flashTimer > 0.0f) {
            renderPostProcessing(renderer, screenW, screenH);
        }
    }
}

void CameraController::renderPostProcessing(SDL_Renderer* renderer, int screenW, int screenH) {
    initSdlPointers();
    if (!renderer || (!fn_SDL_SetRenderDrawBlendMode || !fn_SDL_SetRenderDrawColor || !fn_SDL_RenderFillRect)) {
        return;
    }

    SDL_Rect fullScreen = { 0, 0, screenW, screenH };

    // 1. Color Filter / Tint
    if (m_transform.filterEnabled && m_transform.filterColor.a > 0) {
        int blendMode = static_cast<int>(m_transform.filterBlend);
        fn_SDL_SetRenderDrawBlendMode(renderer, blendMode);
        fn_SDL_SetRenderDrawColor(
            renderer,
            m_transform.filterColor.r,
            m_transform.filterColor.g,
            m_transform.filterColor.b,
            m_transform.filterColor.a
        );
        fn_SDL_RenderFillRect(renderer, &fullScreen);
    }

    // 2. Flash Overlay
    if (m_transform.flashTimer > 0.0f && m_transform.flashDuration > 0.0f) {
        float alphaFactor = m_transform.flashTimer / m_transform.flashDuration;
        uint8_t alpha = static_cast<uint8_t>(m_transform.flashColor.a * alphaFactor);

        if (alpha > 0) {
            fn_SDL_SetRenderDrawBlendMode(renderer, 1); // SDL_BLENDMODE_BLEND
            fn_SDL_SetRenderDrawColor(
                renderer,
                m_transform.flashColor.r,
                m_transform.flashColor.g,
                m_transform.flashColor.b,
                alpha
            );
            fn_SDL_RenderFillRect(renderer, &fullScreen);
        }
    }
}

} // namespace sense

