#include <api/sense_api.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "ui.hpp"

using namespace sense;

bool DeveloperMenuUI::renderButton(
    SDL_Renderer* renderer,
    const Rect& rect,
    const std::string& label,
    bool isActive,
    Color activeColor,
    Color normalColor
) {
    auto& rc = RenderController::instance();
    bool hovered = (!m_contentHitTest || m_contentViewport.contains(m_mousePos.x, m_mousePos.y)) &&
                   rect.contains(m_mousePos.x, m_mousePos.y);
    bool clicked = hovered && m_mouseClicked;

    Color background = isActive ? activeColor : (hovered ? Color(55, 80, 120, 255) : normalColor);
    Color border = hovered ? Color::Cyan() : (isActive ? Color::White() : Color(70, 90, 130, 255));
    Color text = hovered ? Color::Yellow() : Color::White();

    rc.fillRect(renderer, rect, background);
    rc.drawRect(renderer, rect, border);

    Point textSize = rc.getTextSize(label, 0.9f);
    int textX = rect.x + (rect.w - textSize.x) / 2;
    int textY = rect.y + (rect.h - textSize.y) / 2;
    rc.drawText(renderer, label, textX, textY, text, 0.9f, true);

    return clicked;
}

bool DeveloperMenuUI::renderCheckbox(SDL_Renderer* renderer, const Rect& rect, const std::string& label, bool* value) {
    auto& rc = RenderController::instance();
    bool hovered = (!m_contentHitTest || m_contentViewport.contains(m_mousePos.x, m_mousePos.y)) &&
                   rect.contains(m_mousePos.x, m_mousePos.y);
    bool clicked = hovered && m_mouseClicked;

    if (clicked && value) {
        *value = !*value;
    }

    bool checked = value && *value;
    Rect boxRect(rect.x, rect.y + 2, 18, 18);
    Color background = checked ? Color::Green() : (hovered ? Color(50, 60, 80, 255) : Color(30, 35, 50, 255));
    Color border = hovered ? Color::Cyan() : Color::White();

    rc.fillRect(renderer, boxRect, background);
    rc.drawRect(renderer, boxRect, border);
    rc.drawText(renderer, label, rect.x + 26, rect.y + 3, hovered ? Color::Yellow() : Color::White(), 0.9f, true);

    return clicked;
}

bool DeveloperMenuUI::renderSlider(
    SDL_Renderer* renderer,
    const Rect& rect,
    const std::string& label,
    float* value,
    float minVal,
    float maxVal,
    float step
) {
    auto& rc = RenderController::instance();
    bool hovered = (!m_contentHitTest || m_contentViewport.contains(m_mousePos.x, m_mousePos.y)) &&
                   rect.contains(m_mousePos.x, m_mousePos.y);

    constexpr int labelWidth = 180;
    rc.drawText(renderer, label, rect.x, rect.y + 2, hovered ? Color::Yellow() : Color::White(), 0.9f, true);

    Rect barRect(rect.x + labelWidth, rect.y + 3, rect.w - labelWidth - 60, 16);
    rc.fillRect(renderer, barRect, Color(25, 30, 45, 255));
    rc.drawRect(renderer, barRect, hovered ? Color::Cyan() : Color::Gray(160));

    if ((m_mouseDown || m_mouseClicked) && barRect.contains(m_mousePos.x, m_mousePos.y) && value) {
        float factor = static_cast<float>(m_mousePos.x - barRect.x) / static_cast<float>(barRect.w);
        *value = minVal + std::clamp(factor, 0.0f, 1.0f) * (maxVal - minVal);
    }

    float currentValue = value ? *value : minVal;
    float factor = maxVal > minVal ? (currentValue - minVal) / (maxVal - minVal) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);

    int fillWidth = static_cast<int>(barRect.w * factor);
    if (fillWidth > 0) {
        rc.fillRect(renderer, Rect(barRect.x, barRect.y, fillWidth, barRect.h), Color::Cyan());
    }

    std::ostringstream text;
    text << std::fixed << std::setprecision(1) << currentValue;
    rc.drawText(renderer, text.str(), barRect.x + barRect.w + 10, rect.y + 3, Color::White(), 0.9f, true);

    return hovered && m_mouseClicked;
}

// ============================================================================
// Event Handling
// ============================================================================
bool DeveloperMenuUI::handleEvent(const SDL_Event& event) {
    if (!m_visible) return false;

    if (event.type == 0x403) { // SDL_MOUSEWHEEL
        if (m_contentViewport.contains(m_mousePos.x, m_mousePos.y)) {
            if (m_activeTab == 4) {
                m_modScrollY = std::clamp(m_modScrollY - event.wheel.y * 36, 0, m_maxModScrollY);
            } else {
                m_contentScrollY = std::clamp(m_contentScrollY - event.wheel.y * 36, 0, m_maxContentScrollY);
            }
            return true;
        }
    }

    if (event.type == 0x400) { // SDL_MOUSEMOTION
        m_mousePos = Point(event.motion.x, event.motion.y);
        if (m_dragging) {
            m_windowRect.x = m_mousePos.x - m_dragOffset.x;
            m_windowRect.y = m_mousePos.y - m_dragOffset.y;
            return true;
        }
    }
    else if (event.type == 0x401) { // SDL_MOUSEBUTTONDOWN
        if (event.button.button == 1) { // Left click
            m_mousePos = Point(event.button.x, event.button.y);
            m_mouseDown = true;
            m_mouseClicked = true;

            Rect titleBar(m_windowRect.x, m_windowRect.y, m_windowRect.w, 32);
            if (titleBar.contains(m_mousePos.x, m_mousePos.y)) {
                // Check close button click
                Rect closeBtn(m_windowRect.x + m_windowRect.w - 28, m_windowRect.y + 4, 24, 24);
                if (closeBtn.contains(m_mousePos.x, m_mousePos.y)) {
                    m_visible = false;
                    return true;
                }
                m_dragging = true;
                m_dragOffset = Point(m_mousePos.x - m_windowRect.x, m_mousePos.y - m_windowRect.y);
                return true;
            }

            if (m_windowRect.contains(m_mousePos.x, m_mousePos.y)) {
                return true; // Eat mouse click inside window
            }
        }
    }
    else if (event.type == 0x402) { // SDL_MOUSEBUTTONUP
        if (event.button.button == 1) {
            m_mouseDown = false;
            m_dragging = false;
            m_scrollDragging = false;
        }
    }

    if (event.type == 0x400) {
        return m_windowRect.contains(event.motion.x, event.motion.y);
    }
    if (event.type == 0x401 || event.type == 0x402) {
        return m_windowRect.contains(event.button.x, event.button.y);
    }
    return false;
}

// ============================================================================
// Rendering
// ============================================================================
void DeveloperMenuUI::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_visible || !renderer) return;

    // 0. Boundary Clamping: Ensure Developer UI window can never appear off-screen
    if (screenW > 0 && screenH > 0) {
        m_windowRect.w = std::min(580, std::max(0, screenW - 20));
        m_windowRect.h = std::min(540, std::max(0, screenH - 20));
        m_windowRect.x = std::clamp(m_windowRect.x, 10, std::max(10, screenW - m_windowRect.w - 10));
        m_windowRect.y = std::clamp(m_windowRect.y, 10, std::max(10, screenH - m_windowRect.h - 10));
    }

    auto& rc = RenderController::instance();
    auto& player = PlayerController::instance();
    auto& game = GameController::instance();
    auto& cam = CameraController::instance();

    // 1. Window Shadow & Body
    Rect shadowRect(m_windowRect.x + 6, m_windowRect.y + 6, m_windowRect.w, m_windowRect.h);
    rc.fillRect(renderer, shadowRect, Color(0, 0, 0, 140));

    rc.fillRect(renderer, m_windowRect, Color(16, 20, 30, 248));
    rc.drawRect(renderer, m_windowRect, Color(45, 120, 240, 230));

    // 2. Title Bar
    Rect titleRect(m_windowRect.x, m_windowRect.y, m_windowRect.w, 32);
    rc.fillRect(renderer, titleRect, Color(24, 34, 54, 255));
    rc.drawText(renderer, "SENSE MODLOADER  [F1]", m_windowRect.x + 12, m_windowRect.y + 8, Color::Cyan(), 1.0f);

    // Close button [X]
    Rect closeRect(m_windowRect.x + m_windowRect.w - 28, m_windowRect.y + 5, 22, 22);
    bool closeHovered = closeRect.contains(m_mousePos.x, m_mousePos.y);
    rc.fillRect(renderer, closeRect, closeHovered ? Color(200, 40, 40, 255) : Color(60, 30, 30, 200));
    rc.drawRect(renderer, closeRect, Color::White());
    rc.drawText(renderer, "X", closeRect.x + 6, closeRect.y + 3, Color::White(), 1.0f);

    // 3. Navigation Tabs
    const char* tabNames[5] = { "Dashboard", "Cheats", "Camera & FX", "Teleport", "Mods" };
    int tabW = m_windowRect.w / 5;
    int tabY = m_windowRect.y + 36;

    for (int t = 0; t < 5; ++t) {
        Rect tabRect(m_windowRect.x + t * tabW, tabY, tabW, 28);
        bool isCurrent = (m_activeTab == t);
        if (renderButton(renderer, tabRect, tabNames[t], isCurrent, Color(45, 90, 160), Color(25, 30, 45))) {
            if (m_activeTab != t) {
                m_activeTab = t;
                m_contentScrollY = 0;
                m_maxContentScrollY = 0;
                m_modScrollY = 0;
            }
        }
    }

    // 4. Tab Content
    m_contentViewport = Rect(
        m_windowRect.x + 10,
        m_windowRect.y + 78,
        std::max(0, m_windowRect.w - 20),
        std::max(0, m_windowRect.h - 93)
    );

    int contentHeight = 0;
    if (m_activeTab == 0) contentHeight = 298;
    else if (m_activeTab == 1) contentHeight = 310;
    else if (m_activeTab == 2) contentHeight = 386;
    else if (m_activeTab == 3) contentHeight = 220;

    m_maxContentScrollY = std::max(0, contentHeight - m_contentViewport.h);
    m_contentScrollY = std::clamp(m_contentScrollY, 0, m_maxContentScrollY);
    int contentY = m_contentViewport.y - m_contentScrollY;

    typedef int (*PFN_SDL_RenderSetClipRect)(SDL_Renderer*, const SDL_Rect*);
    static PFN_SDL_RenderSetClipRect fn_SDL_RenderSetClipRect = nullptr;
    if (!fn_SDL_RenderSetClipRect) {
        HMODULE hSdl = GetModuleHandleA("SDL2.dll");
        if (hSdl) {
            fn_SDL_RenderSetClipRect = reinterpret_cast<PFN_SDL_RenderSetClipRect>(
                GetProcAddress(hSdl, "SDL_RenderSetClipRect")
            );
        }
    }

    SDL_Rect contentClip{
        m_contentViewport.x,
        m_contentViewport.y,
        m_contentViewport.w,
        m_contentViewport.h
    };
    if (fn_SDL_RenderSetClipRect) {
        fn_SDL_RenderSetClipRect(renderer, &contentClip);
    }
    m_contentHitTest = true;

    if (m_activeTab == 0) { // Dashboard
        std::ostringstream ss;
        ss << "Status:       " << (player.isValid() ? "CONNECTED TO GAME" : "SEARCHING FOR PLAYER...") << "\n\n";
        ss << "Position Y:   " << player.getY() << " px\n";
        ss << "Speed:        " << std::fixed << std::setprecision(1) << player.getSpeed() << " (Normal: " << player.getSpeedNormal() << ")\n";
        ss << "Balance:      " << std::setprecision(2) << player.getBalance() << (player.isBalanced() ? " [STABLE]" : " [CRITICAL!]") << "\n";
        ss << "Moving:       " << (player.isMoving() ? "YES" : "NO") << "\n";
        ss << "Has Lost:     " << (player.hasLost() ? "TRUE (GAME OVER)" : "FALSE") << "\n\n";
        ss << "FPS:          " << std::setprecision(1) << game.getFPS() << "\n";
        ss << "TimeScale:    " << std::setprecision(2) << game.getTimeScale() << "x\n";
        ss << "Endless Mode: " << (game.isEndlessMode() ? "ENABLED" : "DISABLED") << "\n";

        rc.drawText(renderer, ss.str(), m_windowRect.x + 24, contentY, Color::White(), 1.0f);

        // Quick action buttons
        int btnY = contentY + 230;
        if (renderButton(renderer, Rect(m_windowRect.x + 20, btnY, 250, 30), player.isGodMode() ? "GOD MODE: [ON]" : "GOD MODE: [OFF]", player.isGodMode())) {
            player.setGodMode(!player.isGodMode());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, btnY, 250, 30), player.isAutoBalance() ? "AUTO-BALANCE: [ON]" : "AUTO-BALANCE: [OFF]", player.isAutoBalance())) {
            player.setAutoBalance(!player.isAutoBalance());
        }

        btnY += 38;
        if (renderButton(renderer, Rect(m_windowRect.x + 20, btnY, 250, 30), game.isEndlessMode() ? "ENDLESS MODE: [ON]" : "ENDLESS MODE: [OFF]", game.isEndlessMode())) {
            game.toggleEndlessMode();
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, btnY, 250, 30), "SOFT RESET GAME", false, Color::Red(), Color(160, 60, 30))) {
            game.restart();
            rc.showNotification("Game softly reset!", 2.0f, Color::Yellow());
        }
    }
    else if (m_activeTab == 1) { // Cheats
        rc.drawText(renderer, "Player Cheats & Automations:", m_windowRect.x + 20, contentY, Color::Yellow(), 1.0f);

        // God Mode & Auto Balance
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 20, 250, 30), player.isGodMode() ? "GOD MODE (F4): [ON]" : "GOD MODE (F4): [OFF]", player.isGodMode())) {
            player.setGodMode(!player.isGodMode());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, contentY + 20, 250, 30), player.isAutoBalance() ? "AUTO-BALANCE (F5): [ON]" : "AUTO-BALANCE (F5): [OFF]", player.isAutoBalance())) {
            player.setAutoBalance(!player.isAutoBalance());
        }

        // Freeze & Revive
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 58, 250, 30), player.isFrozen() ? "FREEZE: [ACTIVE]" : "FREEZE: [OFF]", player.isFrozen(), Color(180, 50, 50))) {
            player.freeze(!player.isFrozen());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, contentY + 58, 250, 30), "REVIVE / CLEAR DEFEAT", false, Color::Green(), Color(30, 90, 60))) {
            player.resetDefeat();
            rc.showNotification("Player revived!", 2.0f, Color::Green());
        }

        // Endless Mode Toggle
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 96, 250, 30), game.isEndlessMode() ? "ENDLESS MODE (Space): [ON]" : "ENDLESS MODE (Space): [OFF]", game.isEndlessMode())) {
            game.toggleEndlessMode();
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, contentY + 96, 250, 30), "STEP FORWARD", false, Color::Cyan(), Color(30, 60, 90))) {
            player.step(PlayerMove::Left);
        }

        // Warping
        rc.drawText(renderer, "Warping & Position Teleport:", m_windowRect.x + 20, contentY + 138, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 156, 120, 28), "WARP +500")) {
            player.teleport(player.getY() + 500);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 150, contentY + 156, 120, 28), "WARP +1000 (F7)")) {
            player.teleport(player.getY() + 1000);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 280, contentY + 156, 120, 28), "WARP +5000")) {
            player.teleport(player.getY() + 5000);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 410, contentY + 156, 130, 28), "WARP TO END (F12)")) {
            player.teleportToCheckpoint(CheckPoint::FINAL_START);
        }

        // TimeScale
        rc.drawText(renderer, "Time Scale (Slow-Mo):", m_windowRect.x + 20, contentY + 198, Color::Yellow(), 1.0f);
        float curTs = game.getTimeScale();
        float tsValues[3] = { 0.25f, 0.5f, 1.0f };
        const char* tsLabels[3] = { "0.25x", "0.5x", "1.0x" };

        for (int i = 0; i < 3; ++i) {
            Rect tsRect(m_windowRect.x + 20 + i * 88, contentY + 216, 80, 26);
            bool isActive = (std::abs(curTs - tsValues[i]) < 0.05f);
            if (renderButton(renderer, tsRect, tsLabels[i], isActive, Color(50, 120, 200), Color(30, 40, 60))) {
                game.setTimeScale(tsValues[i]);
                rc.showNotification(std::string("Time Scale: ") + tsLabels[i], 1.5f, Color::Magenta());
            }
        }

        // Reset Options
        rc.drawText(renderer, "Reset Game:", m_windowRect.x + 20, contentY + 258, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 276, 250, 32), "SOFT RESET (0 px)", false, Color::Red(), Color(160, 60, 30))) {
            game.restart();
            rc.showNotification("Soft reset executed!", 2.0f, Color::Yellow());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 290, contentY + 276, 250, 32), "HARD RELOAD", false, Color::Red(), Color(140, 40, 40))) {
            game.killAndReload();
            rc.showNotification("Hard reload triggered!", 2.0f, Color::Red());
        }
    }
    else if (m_activeTab == 2) { // Camera & FX
        // 1. 3D Perspective Presets
        rc.drawText(renderer, "3D Perspective Presets (True Mode-7 / Projection):", m_windowRect.x + 20, contentY, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 18, 125, 26), "3D HIGHWAY", std::abs(cam.getPerspectiveTilt().y - 0.42f) < 0.05f)) {
            cam.preset3DHighway();
            rc.showNotification("3D Highway Mode-7 Perspective [ACTIVE]", 2.0f, Color::Green());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 155, contentY + 18, 120, 26), "BIRD'S EYE", std::abs(cam.getPerspectiveTilt().y - (-0.35f)) < 0.05f)) {
            cam.presetBirdEye();
            rc.showNotification("Bird's Eye Perspective [ACTIVE]", 2.0f, Color::Green());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 285, contentY + 18, 120, 26), "ISOMETRIC", std::abs(cam.getPerspectiveTilt().x - 0.28f) < 0.05f)) {
            cam.presetIsometric();
            rc.showNotification("Isometric 3D Perspective [ACTIVE]", 2.0f, Color::Green());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 415, contentY + 18, 125, 26), "NORMAL 2D", (std::abs(cam.getPerspectiveTilt().x) < 0.01f && std::abs(cam.getPerspectiveTilt().y) < 0.01f && std::abs(cam.getAngle()) < 0.01f))) {
            cam.reset();
            rc.showNotification("Normal 2D Camera Restored", 1.5f, Color::White());
        }

        // 2. Fine 3D Perspective Tilt Controls
        PointF curTilt = cam.getPerspectiveTilt();
        rc.drawText(renderer, "Manual 3D Tilt (Pitch / Yaw):", m_windowRect.x + 20, contentY + 50, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 68, 125, 26), "PITCH +0.10")) {
            cam.setPerspectiveTilt(curTilt.x, curTilt.y + 0.10f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 155, contentY + 68, 120, 26), "PITCH -0.10")) {
            cam.setPerspectiveTilt(curTilt.x, curTilt.y - 0.10f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 285, contentY + 68, 120, 26), "YAW +0.10")) {
            cam.setPerspectiveTilt(curTilt.x + 0.10f, curTilt.y);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 415, contentY + 68, 125, 26), "YAW -0.10")) {
            cam.setPerspectiveTilt(curTilt.x - 0.10f, curTilt.y);
        }

        // 3. Rotation Angle & Flipping
        rc.drawText(renderer, "Rotation Roll & Mirror:", m_windowRect.x + 20, contentY + 100, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 118, 80, 26), "0°", std::abs(cam.getAngle()) < 0.1f)) {
            cam.setAngle(0.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 106, contentY + 118, 80, 26), "+15°", std::abs(cam.getAngle() - 15.0f) < 0.1f)) {
            cam.setAngle(15.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 192, contentY + 118, 80, 26), "-15°", std::abs(cam.getAngle() - (-15.0f)) < 0.1f)) {
            cam.setAngle(-15.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 278, contentY + 118, 80, 26), "45°", std::abs(cam.getAngle() - 45.0f) < 0.1f)) {
            cam.setAngle(45.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 364, contentY + 118, 80, 26), "180°", std::abs(cam.getAngle() - 180.0f) < 0.1f)) {
            cam.setAngle(180.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 450, contentY + 118, 90, 26), "MIRROR", cam.isFlippedH())) {
            cam.setFlipped(!cam.isFlippedH(), cam.isFlippedV());
        }

        // 4. Centered Zoom Levels
        rc.drawText(renderer, "Camera Centered Zoom:", m_windowRect.x + 20, contentY + 150, Color::Yellow(), 1.0f);
        float curZoom = cam.getZoom();
        float zValues[6] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
        const char* zLabels[6] = { "0.5x", "0.75x", "1.0x", "1.25x", "1.5x", "2.0x" };
        for (int i = 0; i < 6; ++i) {
            Rect zRect(m_windowRect.x + 20 + i * 88, contentY + 168, 80, 26);
            bool isActive = (std::abs(curZoom - zValues[i]) < 0.05f);
            if (renderButton(renderer, zRect, zLabels[i], isActive, Color(50, 120, 200), Color(30, 40, 60))) {
                cam.setZoom(zValues[i]);
            }
        }

        // 5. Freecam Pan Offset
        rc.drawText(renderer, "Pan Offset (Freecam Position):", m_windowRect.x + 20, contentY + 200, Color::Yellow(), 1.0f);
        PointF curOffset = cam.getOffset();
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 218, 100, 26), "LEFT (-60)")) {
            cam.setOffset(curOffset.x - 60.0f, curOffset.y);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 128, contentY + 218, 100, 26), "RIGHT (+60)")) {
            cam.setOffset(curOffset.x + 60.0f, curOffset.y);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 236, contentY + 218, 100, 26), "UP (-40)")) {
            cam.setOffset(curOffset.x, curOffset.y - 40.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 344, contentY + 218, 100, 26), "DOWN (+40)")) {
            cam.setOffset(curOffset.x, curOffset.y + 40.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 452, contentY + 218, 88, 26), "CENTER")) {
            cam.setOffset(0.0f, 0.0f);
        }

        // 6. Screen Shake
        rc.drawText(renderer, "Screen Shake:", m_windowRect.x + 20, contentY + 250, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 268, 160, 26), "LIGHT SHAKE (0.5s)", false, Color::Red(), Color(120, 50, 50))) {
            cam.shake(0.5f, 15.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 190, contentY + 268, 160, 26), "HEAVY SHAKE (1.5s)", false, Color::Red(), Color(150, 40, 40))) {
            cam.shake(1.5f, 35.0f);
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 360, contentY + 268, 160, 26), "STOP SHAKE", false, Color::Gray(100), Color(50, 60, 70))) {
            cam.stopShake();
        }

        // 7. Visual Filters
        rc.drawText(renderer, "Visual Post-Processing Filters:", m_windowRect.x + 20, contentY + 300, Color::Yellow(), 1.0f);
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 318, 125, 26), "NIGHT VISION", false, Color::Green(), Color(20, 80, 40))) {
            cam.presetNightVision();
            rc.showNotification("Night Vision filter applied!", 1.5f, Color::Green());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 155, contentY + 318, 120, 26), "SEPIA TONE", false, Color::Yellow(), Color(90, 60, 30))) {
            cam.presetSepia();
            rc.showNotification("Sepia filter applied!", 1.5f, Color::Yellow());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 285, contentY + 318, 120, 26), "CYBERPUNK", false, Color::Magenta(), Color(100, 20, 80))) {
            cam.presetCyberpunk();
            rc.showNotification("Cyberpunk filter applied!", 1.5f, Color::Magenta());
        }
        if (renderButton(renderer, Rect(m_windowRect.x + 415, contentY + 318, 125, 26), "CLEAR FILTER", false, Color::White(), Color(40, 50, 70))) {
            cam.clearColorFilter();
            rc.showNotification("Visual filter cleared!", 1.5f, Color::White());
        }

        // 8. Reset All Camera Settings
        if (renderButton(renderer, Rect(m_windowRect.x + 20, contentY + 356, 520, 30), "RESET ALL CAMERA & PERSPECTIVE SETTINGS", false, Color::Cyan(), Color(40, 70, 110))) {
            cam.reset();
            rc.showNotification("Camera & Perspective reset to default!", 2.0f, Color::Cyan());
        }
    }
    else if (m_activeTab == 3) { // Teleport
        rc.drawText(renderer, "Select Track Checkpoint to Teleport:", m_windowRect.x + 20, contentY, Color::Yellow(), 1.0f);

        struct CPBtn { const char* label; int y; };
        CPBtn cpList[12] = {
            { "Start (0 px)", 0 },           { "CP A (500 px)", 500 },
            { "CP B (1500 px)", 1500 },      { "CP D (3500 px)", 3500 },
            { "CP F (5500 px)", 5500 },      { "CP H (7500 px)", 7500 },
            { "CP K (10500 px)", 10500 },    { "CP M (12500 px)", 12500 },
            { "CP P (15500 px)", 15500 },    { "CP R (17500 px)", 17500 },
            { "CP T (19500 px)", 19500 },    { "Final (25000 px)", 25000 }
        };

        for (int i = 0; i < 12; ++i) {
            int col = i % 2;
            int row = i / 2;
            int bx = m_windowRect.x + 20 + col * 270;
            int by = contentY + 22 + row * 34;

            Rect btnRect(bx, by, 250, 28);
            if (renderButton(renderer, btnRect, cpList[i].label, false, Color::Cyan(), Color(30, 45, 70))) {
                player.teleport(cpList[i].y);
                rc.showNotification(std::string("Teleported to ") + cpList[i].label, 2.0f, Color::Cyan());
            }
        }
    }
    else if (m_activeTab == 4) { // Custom Mods
        const auto& widgets = UIController::instance().getWidgets();
        if (widgets.empty()) {
            rc.drawText(renderer, "No custom widgets registered by active mods.", m_windowRect.x + 20, contentY + 20, Color::Gray(160), 1.0f);
        } else {
            int contentVisibleH = m_contentViewport.h;

            // Compute total height of all widgets
            int totalH = 20;
            for (const auto& w : widgets) {
                if (w.type == UIWidgetType::Button) totalH += 34;
                else if (w.type == UIWidgetType::Checkbox) totalH += 30;
                else if (w.type == UIWidgetType::Slider) totalH += 30;
                else if (w.type == UIWidgetType::Label) totalH += 22;
                else if (w.type == UIWidgetType::Section) totalH += 28;
            }

            m_maxModScrollY = std::max(0, totalH - contentVisibleH);
            m_modScrollY = std::clamp(m_modScrollY, 0, m_maxModScrollY);

            // Handle scrollbar dragging
            Rect trackRect(m_windowRect.x + m_windowRect.w - 14, m_contentViewport.y, 8, contentVisibleH);
            if (m_maxModScrollY > 0) {
                if (m_mouseClicked && trackRect.contains(m_mousePos.x, m_mousePos.y)) {
                    m_scrollDragging = true;
                }
                if (m_scrollDragging && m_mouseDown && contentVisibleH > 0) {
                    float dragRatio = static_cast<float>(m_mousePos.y - m_contentViewport.y) / static_cast<float>(contentVisibleH);
                    m_modScrollY = std::clamp(static_cast<int>(dragRatio * m_maxModScrollY), 0, m_maxModScrollY);
                }
            }

            int widgetW = (m_maxModScrollY > 0) ? (m_windowRect.w - 46) : (m_windowRect.w - 40);
            int wy = contentY + 10 - m_modScrollY;

            for (const auto& w : widgets) {
                int widgetH = (w.type == UIWidgetType::Button) ? 28 :
                              (w.type == UIWidgetType::Checkbox || w.type == UIWidgetType::Slider) ? 24 :
                              (w.type == UIWidgetType::Label) ? 20 : 26;

                bool inView = (wy + widgetH >= m_contentViewport.y &&
                               wy <= m_contentViewport.y + contentVisibleH);

                if (w.type == UIWidgetType::Button) {
                    if (inView) {
                        if (renderButton(renderer, Rect(m_windowRect.x + 20, wy, std::min(widgetW, 300), 28), w.label)) {
                            if (w.onClick) w.onClick();
                        }
                    }
                    wy += 34;
                }
                else if (w.type == UIWidgetType::Checkbox) {
                    if (inView) {
                        renderCheckbox(renderer, Rect(m_windowRect.x + 20, wy, std::min(widgetW, 360), 24), w.label, w.boolValue);
                    }
                    wy += 30;
                }
                else if (w.type == UIWidgetType::Slider) {
                    if (inView) {
                        renderSlider(renderer, Rect(m_windowRect.x + 20, wy, widgetW, 24), w.label, w.floatValue, w.minVal, w.maxVal, w.step);
                    }
                    wy += 30;
                }
                else if (w.type == UIWidgetType::Label) {
                    if (inView) {
                        rc.drawText(renderer, w.label, m_windowRect.x + 20, wy, Color::White(), 1.0f);
                    }
                    wy += 22;
                }
                else if (w.type == UIWidgetType::Section) {
                    if (inView) {
                        rc.drawText(renderer, "--- " + w.label + " ---", m_windowRect.x + 20, wy, Color::Yellow(), 1.0f);
                    }
                    wy += 28;
                }
            }

            // Draw scrollbar
            if (m_maxModScrollY > 0) {
                rc.fillRect(renderer, trackRect, Color(20, 25, 35, 180));
                rc.drawRect(renderer, trackRect, Color(40, 50, 70, 200));

                float thumbRatio = static_cast<float>(contentVisibleH) / static_cast<float>(totalH);
                int thumbH = std::min(contentVisibleH, std::max(24, static_cast<int>(contentVisibleH * thumbRatio)));
                float scrollRatio = static_cast<float>(m_modScrollY) / static_cast<float>(m_maxModScrollY);
                int thumbY = m_contentViewport.y + static_cast<int>((contentVisibleH - thumbH) * scrollRatio);

                Rect thumbRect(trackRect.x, thumbY, trackRect.w, thumbH);
                rc.fillRect(renderer, thumbRect, Color(70, 140, 220, 240));
                rc.drawRect(renderer, thumbRect, Color::White());
            }
        }
    }

    m_contentHitTest = false;
    if (fn_SDL_RenderSetClipRect) {
        fn_SDL_RenderSetClipRect(renderer, nullptr);
    }

    if (m_activeTab != 4 && m_maxContentScrollY > 0 && m_contentViewport.h > 0) {
        Rect trackRect(m_windowRect.x + m_windowRect.w - 12, m_contentViewport.y, 5, m_contentViewport.h);
        rc.fillRect(renderer, trackRect, Color(20, 25, 35, 180));
        rc.drawRect(renderer, trackRect, Color(40, 50, 70, 200));

        int thumbHeight = std::min(m_contentViewport.h, std::max(
            24,
            static_cast<int>(static_cast<float>(m_contentViewport.h) * m_contentViewport.h / contentHeight)
        ));
        int scrollRange = m_contentViewport.h - thumbHeight;
        int thumbY = trackRect.y + (m_maxContentScrollY > 0
            ? scrollRange * m_contentScrollY / m_maxContentScrollY
            : 0);
        Rect thumbRect(trackRect.x, thumbY, trackRect.w, thumbHeight);
        rc.fillRect(renderer, thumbRect, Color(70, 140, 220, 240));
        rc.drawRect(renderer, thumbRect, Color::White());
    }

    // Consume single click at end of frame
    m_mouseClicked = false;
}
