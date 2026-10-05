#include "ui.hpp"
#include "render.hpp"
#include "player.hpp"
#include "game.hpp"
#include "camera.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace sense {

UIController::UIController() = default;

UIController& UIController::instance() {
    static UIController inst;
    return inst;
}

bool UIController::isVisible() const {
    return m_visible;
}

void UIController::setVisible(bool visible) {
    m_visible = visible;
}

void UIController::toggle() {
    m_visible = !m_visible;
}

void UIController::addButton(const std::string& label, std::function<void()> onClick) {
    UIWidget w;
    w.type = UIWidgetType::Button;
    w.label = label;
    w.onClick = onClick;
    m_modWidgets.push_back(w);
}

void UIController::addCheckbox(const std::string& label, bool* boolValue) {
    UIWidget w;
    w.type = UIWidgetType::Checkbox;
    w.label = label;
    w.boolValue = boolValue;
    m_modWidgets.push_back(w);
}

void UIController::addSlider(const std::string& label, float* floatValue, float minVal, float maxVal, float step) {
    UIWidget w;
    w.type = UIWidgetType::Slider;
    w.label = label;
    w.floatValue = floatValue;
    w.minVal = minVal;
    w.maxVal = maxVal;
    w.step = step;
    m_modWidgets.push_back(w);
}

void UIController::addLabel(const std::string& text) {
    UIWidget w;
    w.type = UIWidgetType::Label;
    w.label = text;
    m_modWidgets.push_back(w);
}

void UIController::addSection(const std::string& title) {
    UIWidget w;
    w.type = UIWidgetType::Section;
    w.label = title;
    m_modWidgets.push_back(w);
}

// ============================================================================
// Immediate-Mode UI Helpers
// ============================================================================
bool UIController::renderButton(
    SDL_Renderer* renderer,
    const Rect& rect,
    const std::string& label,
    bool isActive,
    Color activeColor,
    Color normalColor
) {
    auto& rc = RenderController::instance();
    bool hovered = rect.contains(m_mousePos.x, m_mousePos.y);
    bool clicked = hovered && m_mouseClicked;

    Color bg = isActive ? activeColor : (hovered ? Color(55, 80, 120, 255) : normalColor);
    Color border = hovered ? Color::Cyan() : (isActive ? Color::White() : Color(70, 90, 130, 255));
    Color textCol = hovered ? Color::Yellow() : Color::White();

    rc.fillRect(renderer, rect, bg);
    rc.drawRect(renderer, rect, border);

    Point txtSize = rc.getTextSize(label, 0.9f);
    int tx = rect.x + (rect.w - txtSize.x) / 2;
    int ty = rect.y + (rect.h - txtSize.y) / 2;
    rc.drawText(renderer, label, tx, ty, textCol, 0.9f, true);

    return clicked;
}

bool UIController::renderCheckbox(SDL_Renderer* renderer, const Rect& rect, const std::string& label, bool* value) {
    auto& rc = RenderController::instance();
    bool hovered = rect.contains(m_mousePos.x, m_mousePos.y);
    bool clicked = hovered && m_mouseClicked;

    if (clicked && value) {
        *value = !(*value);
    }

    bool val = value ? *value : false;
    Rect boxRect(rect.x, rect.y + 2, 18, 18);

    Color boxBg = val ? Color::Green() : (hovered ? Color(50, 60, 80, 255) : Color(30, 35, 50, 255));
    Color border = hovered ? Color::Cyan() : Color::White();

    rc.fillRect(renderer, boxRect, boxBg);
    rc.drawRect(renderer, boxRect, border);

    Color textCol = hovered ? Color::Yellow() : Color::White();
    rc.drawText(renderer, label, rect.x + 26, rect.y + 3, textCol, 0.9f, true);

    return clicked;
}

bool UIController::renderSlider(SDL_Renderer* renderer, const Rect& rect, const std::string& label, float* value, float minVal, float maxVal, float step) {
    auto& rc = RenderController::instance();
    bool hovered = rect.contains(m_mousePos.x, m_mousePos.y);

    int labelW = 180;
    rc.drawText(renderer, label, rect.x, rect.y + 2, hovered ? Color::Yellow() : Color::White(), 0.9f, true);

    Rect barRect(rect.x + labelW, rect.y + 3, rect.w - labelW - 60, 16);
    rc.fillRect(renderer, barRect, Color(25, 30, 45, 255));
    rc.drawRect(renderer, barRect, hovered ? Color::Cyan() : Color::Gray(160));

    if ((m_mouseDown || m_mouseClicked) && barRect.contains(m_mousePos.x, m_mousePos.y) && value) {
        float factor = static_cast<float>(m_mousePos.x - barRect.x) / static_cast<float>(barRect.w);
        factor = std::clamp(factor, 0.0f, 1.0f);
        *value = minVal + factor * (maxVal - minVal);
    }

    float currentVal = value ? *value : minVal;
    float factor = (maxVal > minVal) ? ((currentVal - minVal) / (maxVal - minVal)) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);

    int fillW = static_cast<int>(barRect.w * factor);
    if (fillW > 0) {
        rc.fillRect(renderer, Rect(barRect.x, barRect.y, fillW, barRect.h), Color::Cyan());
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << currentVal;
    rc.drawText(renderer, ss.str(), barRect.x + barRect.w + 10, rect.y + 3, Color::White(), 0.9f, true);

    return hovered && m_mouseClicked;
}

// ============================================================================
// Event Handling
// ============================================================================
bool UIController::handleEvent(const SDL_Event& event) {
    if (event.type == 0x300) { // SDL_KEYDOWN
        if (event.key.keysym.sym == Key_F1) {
            toggle();
            return true;
        }
    }

    if (!m_visible) return false;

    if (event.type == 0x403) { // SDL_MOUSEWHEEL
        if (m_visible && m_windowRect.contains(m_mousePos.x, m_mousePos.y)) {
            m_modScrollY -= event.wheel.y * 36;
            if (m_modScrollY < 0) m_modScrollY = 0;
            if (m_modScrollY > m_maxModScrollY) m_modScrollY = m_maxModScrollY;
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

    return m_windowRect.contains(event.motion.x, event.motion.y);
}

// ============================================================================
// Rendering
// ============================================================================
void UIController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_visible || !renderer) return;

    // 0. Boundary Clamping: Ensure Developer UI window can never appear off-screen
    if (screenW > 0 && screenH > 0) {
        if (m_windowRect.w > screenW - 20) m_windowRect.w = screenW - 20;
        if (m_windowRect.h > screenH - 20) m_windowRect.h = screenH - 20;
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
    rc.drawText(renderer, "SENSE MODLOADER API  [F1]", m_windowRect.x + 12, m_windowRect.y + 8, Color::Cyan(), 1.0f);

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
                m_modScrollY = 0;
            }
        }
    }

    // 4. Tab Content
    int contentY = m_windowRect.y + 78;

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
        rc.drawText(renderer, "Time Scale (Speedhack / Slow-Mo):", m_windowRect.x + 20, contentY + 198, Color::Yellow(), 1.0f);
        float curTs = game.getTimeScale();
        float tsValues[6] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 5.0f };
        const char* tsLabels[6] = { "0.25x", "0.5x", "1.0x", "1.5x", "2.0x (F6)", "5.0x" };

        for (int i = 0; i < 6; ++i) {
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
        if (m_modWidgets.empty()) {
            rc.drawText(renderer, "No custom widgets registered by active mods.", m_windowRect.x + 20, contentY + 20, Color::Gray(160), 1.0f);
        } else {
            typedef int (*PFN_SDL_RenderSetClipRect)(SDL_Renderer*, const SDL_Rect*);
            static PFN_SDL_RenderSetClipRect fn_SDL_RenderSetClipRect = nullptr;
            if (!fn_SDL_RenderSetClipRect) {
                HMODULE hSdl = GetModuleHandleA("SDL2.dll");
                if (hSdl) {
                    fn_SDL_RenderSetClipRect = reinterpret_cast<PFN_SDL_RenderSetClipRect>(GetProcAddress(hSdl, "SDL_RenderSetClipRect"));
                }
            }

            int contentVisibleH = (m_windowRect.y + m_windowRect.h - 15) - contentY;
            if (contentVisibleH < 50) contentVisibleH = 50;

            // Compute total height of all widgets
            int totalH = 20;
            for (const auto& w : m_modWidgets) {
                if (w.type == UIWidgetType::Button) totalH += 34;
                else if (w.type == UIWidgetType::Checkbox) totalH += 30;
                else if (w.type == UIWidgetType::Slider) totalH += 30;
                else if (w.type == UIWidgetType::Label) totalH += 22;
                else if (w.type == UIWidgetType::Section) totalH += 28;
            }

            m_maxModScrollY = std::max(0, totalH - contentVisibleH);
            m_modScrollY = std::clamp(m_modScrollY, 0, m_maxModScrollY);

            // Handle scrollbar dragging
            Rect trackRect(m_windowRect.x + m_windowRect.w - 14, contentY, 8, contentVisibleH);
            if (m_maxModScrollY > 0) {
                if (m_mouseClicked && trackRect.contains(m_mousePos.x, m_mousePos.y)) {
                    m_scrollDragging = true;
                }
                if (m_scrollDragging && m_mouseDown) {
                    float dragRatio = static_cast<float>(m_mousePos.y - contentY) / static_cast<float>(contentVisibleH);
                    m_modScrollY = std::clamp(static_cast<int>(dragRatio * m_maxModScrollY), 0, m_maxModScrollY);
                }
            }

            // Set clip rect to prevent rendering outside window
            SDL_Rect clipRect;
            clipRect.x = m_windowRect.x + 10;
            clipRect.y = contentY;
            clipRect.w = m_windowRect.w - 20;
            clipRect.h = contentVisibleH;
            if (fn_SDL_RenderSetClipRect) fn_SDL_RenderSetClipRect(renderer, &clipRect);

            int widgetW = (m_maxModScrollY > 0) ? (m_windowRect.w - 46) : (m_windowRect.w - 40);
            int wy = contentY + 10 - m_modScrollY;

            for (auto& w : m_modWidgets) {
                int widgetH = (w.type == UIWidgetType::Button) ? 28 :
                              (w.type == UIWidgetType::Checkbox || w.type == UIWidgetType::Slider) ? 24 :
                              (w.type == UIWidgetType::Label) ? 20 : 26;

                bool inView = (wy + widgetH >= contentY && wy <= contentY + contentVisibleH);

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

            // Restore clipping
            if (fn_SDL_RenderSetClipRect) fn_SDL_RenderSetClipRect(renderer, nullptr);

            // Draw scrollbar
            if (m_maxModScrollY > 0) {
                rc.fillRect(renderer, trackRect, Color(20, 25, 35, 180));
                rc.drawRect(renderer, trackRect, Color(40, 50, 70, 200));

                float thumbRatio = static_cast<float>(contentVisibleH) / static_cast<float>(totalH);
                int thumbH = std::max(24, static_cast<int>(contentVisibleH * thumbRatio));
                float scrollRatio = static_cast<float>(m_modScrollY) / static_cast<float>(m_maxModScrollY);
                int thumbY = contentY + static_cast<int>((contentVisibleH - thumbH) * scrollRatio);

                Rect thumbRect(trackRect.x, thumbY, trackRect.w, thumbH);
                rc.fillRect(renderer, thumbRect, Color(70, 140, 220, 240));
                rc.drawRect(renderer, thumbRect, Color::White());
            }
        }
    }

    // 5. Software Mouse Cursor (High Visibility)
    int mx = m_mousePos.x;
    int my = m_mousePos.y;

    // Shadow
    rc.drawLine(renderer, mx + 1, my + 1, mx + 13, my + 13, Color::Black());
    rc.drawLine(renderer, mx + 1, my + 1, mx + 1, my + 17, Color::Black());
    rc.drawLine(renderer, mx + 1, my + 17, mx + 5, my + 13, Color::Black());
    rc.drawLine(renderer, mx + 5, my + 13, mx + 13, my + 13, Color::Black());

    // Cursor Body
    rc.drawLine(renderer, mx, my, mx + 12, my + 12, Color::White());
    rc.drawLine(renderer, mx, my, mx, my + 16, Color::White());
    rc.drawLine(renderer, mx, my + 16, mx + 4, my + 12, Color::White());
    rc.drawLine(renderer, mx + 4, my + 12, mx + 12, my + 12, Color::White());
    rc.fillRect(renderer, Rect(mx + 1, my + 1, 4, 8), Color::White());

    // Consume single click at end of frame
    m_mouseClicked = false;
}

} // namespace sense
