#include "photomode.hpp"
#include "camera.hpp"
#include "player.hpp"
#include "game.hpp"
#include "render.hpp"
#include "lowlevel.hpp"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <windows.h>

namespace sense {

PhotoModeController& PhotoModeController::instance() {
    static PhotoModeController inst;
    return inst;
}

PhotoModeController::PhotoModeController() = default;

void PhotoModeController::toggle() {
    setEnabled(!m_enabled);
}

void PhotoModeController::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;

    auto& p = PlayerController::instance();
    auto& cam = CameraController::instance();

    if (m_enabled) {
        if (p.isValid()) {
            m_savedMovingState = p.isMoving();
            p.setMoving(false);
        }
        m_camOffsetX = 0.0f;
        m_camOffsetY = 0.0f;
        m_zoom = 1.0f;
        m_roll = 0.0f;
        RenderController::instance().showNotification("Photo Mode: [ON] (WASD: Pan, +/-: Zoom, Q/E: Roll, Space: Shoot)", 3.0f, Color::Cyan());
    } else {
        if (p.isValid()) {
            p.setMoving(m_savedMovingState);
        }
        cam.setOffset(0.0f, 0.0f);
        cam.setZoom(1.0f);
        cam.setAngle(0.0f);
        RenderController::instance().showNotification("Photo Mode: [OFF]", 1.5f, Color::Yellow());
    }
}

void PhotoModeController::update(float deltaTime) {
    if (!m_enabled) return;

    auto& cam = CameraController::instance();
    cam.setOffset(m_camOffsetX, m_camOffsetY);
    cam.setZoom(m_zoom);
    cam.setAngle(m_roll);
}

bool PhotoModeController::handleEvent(const SDL_Event& event) {
    if (!m_enabled) return false;

    // SDL_KEYDOWN is 0x300
    if (event.type == 0x300) {
        int32_t key = event.key.keysym.sym;
        if (key == Key_W || key == Key_Up) {
            m_camOffsetY -= 20.0f;
            return true;
        } else if (key == Key_S || key == Key_Down) {
            m_camOffsetY += 20.0f;
            return true;
        } else if (key == Key_A || key == Key_Left) {
            m_camOffsetX -= 20.0f;
            return true;
        } else if (key == Key_D || key == Key_Right) {
            m_camOffsetX += 20.0f;
            return true;
        } else if (key == '=' || key == '+') {
            m_zoom = std::min(3.0f, m_zoom + 0.1f);
            return true;
        } else if (key == '-' || key == '_') {
            m_zoom = std::max(0.3f, m_zoom - 0.1f);
            return true;
        } else if (key == Key_Q) {
            m_roll -= 2.0f;
            return true;
        } else if (key == Key_E) {
            m_roll += 2.0f;
            return true;
        } else if (key == Key_Space) {
            captureScreenshot(lowlevel::getRawRenderer());
            return true;
        } else if (key == Key_Escape || key == Key_4) {
            setEnabled(false);
            return true;
        }
        return true; // Consume other keys while in photo mode
    }

    if (event.type == 0x301) { // SDL_KEYUP
        return true;
    }

    return false;
}

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bfType = 0x4D42; // 'BM'
    uint32_t bfSize = 0;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 54;

    uint32_t biSize = 40;
    int32_t  biWidth = 0;
    int32_t  biHeight = 0;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 32;
    uint32_t biCompression = 0;
    uint32_t biSizeImage = 0;
    int32_t  biXPelsPerMeter = 2835;
    int32_t  biYPelsPerMeter = 2835;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};
#pragma pack(pop)

bool PhotoModeController::captureScreenshot(SDL_Renderer* renderer, const std::string& customPath) {
    if (!renderer) renderer = lowlevel::getRawRenderer();
    if (!renderer) return false;

    Point winSize = GameController::instance().getWindowSize();
    int w = winSize.x;
    int h = winSize.y;
    if (w <= 0 || h <= 0) {
        w = 1280;
        h = 720;
    }

    HMODULE hSdl = GetModuleHandleA("SDL2.dll");
    if (!hSdl) hSdl = LoadLibraryA("SDL2.dll");
    if (!hSdl) return false;

    using PFN_SDL_RenderReadPixels = int(*)(SDL_Renderer*, const void*, uint32_t, void*, int);
    auto pfnRead = reinterpret_cast<PFN_SDL_RenderReadPixels>(GetProcAddress(hSdl, "SDL_RenderReadPixels"));
    if (!pfnRead) return false;

    // Allocate 32-bit pixel buffer
    std::vector<uint8_t> pixels(static_cast<size_t>(w * h * 4), 0);
    constexpr uint32_t SDL_PIX_ARGB8888 = 372645892;
    int res = pfnRead(renderer, nullptr, SDL_PIX_ARGB8888, pixels.data(), w * 4);
    if (res != 0) {
        res = pfnRead(renderer, nullptr, 0, pixels.data(), w * 4);
        if (res != 0) return false;
    }

    CreateDirectoryA("screenshots", NULL);

    std::string outPath = customPath;
    if (outPath.empty()) {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tmNow{};
        localtime_s(&tmNow, &tt);
        char buf[64];
        std::strftime(buf, sizeof(buf), "screenshots/photo_%Y%m%d_%H%M%S.bmp", &tmNow);
        outPath = buf;
    }

    BMPHeader hdr;
    hdr.biWidth = w;
    hdr.biHeight = -h; // Negative height for top-down DIB
    hdr.biSizeImage = static_cast<uint32_t>(pixels.size());
    hdr.bfSize = 54 + hdr.biSizeImage;

    std::ofstream file(outPath, std::ios::binary);
    if (!file) return false;

    file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    file.close();

    RenderController::instance().showNotification("Saved: " + outPath, 2.5f, Color::Green());
    return true;
}

void PhotoModeController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_enabled || !renderer) return;

    auto& rc = RenderController::instance();

    // 1. Rule-of-Thirds Grid
    if (m_showGrid) {
        Color gridColor(255, 255, 255, 60);
        int x1 = screenW / 3;
        int x2 = (2 * screenW) / 3;
        int y1 = screenH / 3;
        int y2 = (2 * screenH) / 3;

        rc.drawLine(renderer, x1, 0, x1, screenH, gridColor);
        rc.drawLine(renderer, x2, 0, x2, screenH, gridColor);
        rc.drawLine(renderer, 0, y1, screenW, y1, gridColor);
        rc.drawLine(renderer, 0, y2, screenW, y2, gridColor);
    }

    // 2. Camera Viewfinder Frame Corner Brackets
    Color frameColor(0, 220, 255, 180);
    int pad = 40;
    int len = 30;
    // Top-Left
    rc.drawLine(renderer, pad, pad, pad + len, pad, frameColor);
    rc.drawLine(renderer, pad, pad, pad, pad + len, frameColor);
    // Top-Right
    rc.drawLine(renderer, screenW - pad, pad, screenW - pad - len, pad, frameColor);
    rc.drawLine(renderer, screenW - pad, pad, screenW - pad, pad + len, frameColor);
    // Bottom-Left
    rc.drawLine(renderer, pad, screenH - pad, pad + len, screenH - pad, frameColor);
    rc.drawLine(renderer, pad, screenH - pad, pad, screenH - pad - len, frameColor);
    // Bottom-Right
    rc.drawLine(renderer, screenW - pad, screenH - pad, screenW - pad - len, screenH - pad, frameColor);
    rc.drawLine(renderer, screenW - pad, screenH - pad, screenW - pad, screenH - pad - len, frameColor);

    // 3. Center crosshair
    int cx = screenW / 2;
    int cy = screenH / 2;
    rc.drawLine(renderer, cx - 8, cy, cx + 8, cy, Color(255, 255, 255, 100));
    rc.drawLine(renderer, cx, cy - 8, cx, cy + 8, Color(255, 255, 255, 100));

    // 4. On-screen controls HUD bar
    if (m_showHud) {
        rc.fillRect(renderer, Rect(0, screenH - 34, screenW, 34), Color(0, 0, 0, 160));
        std::string hudText = "[PHOTO MODE] WASD: Pan | +/-: Zoom (" + std::to_string(static_cast<int>(m_zoom * 100)) + "%) | Q/E: Roll | Space: Shoot | Esc: Exit";
        rc.drawText(renderer, hudText, 20, screenH - 26, Color::Cyan(), 0.9f, true);
    }
}

} // namespace sense
