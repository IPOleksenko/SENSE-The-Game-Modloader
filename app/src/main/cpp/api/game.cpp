#include "game.hpp"
#include "player.hpp"
#include "lowlevel.hpp"
#include <windows.h>

namespace sense {

GameController::GameController() {
    m_baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
}

GameController& GameController::instance() {
    static GameController inst;
    return inst;
}

float GameController::getTimeScale() const {
    return m_timeScale;
}

void GameController::setTimeScale(float scale) {
    m_timeScale = (scale <= 0.01f) ? 0.01f : scale;
}

bool GameController::isPaused() const {
    return m_paused;
}

void GameController::setPaused(bool paused) {
    m_paused = paused;
}

void GameController::togglePause() {
    m_paused = !m_paused;
}

float GameController::getFPS() const {
    return m_fps;
}

float GameController::getDeltaTime() const {
    return m_deltaTime;
}

uint32_t GameController::getFrameCount() const {
    return m_frameCount;
}

Point GameController::getWindowSize() const {
    return m_windowSize;
}

void GameController::setWindowSize(int width, int height) {
    m_windowSize = Point(width, height);
}

bool GameController::isFullscreen() const {
    return m_fullscreen;
}

void GameController::setFullscreen(bool fullscreen) {
    m_fullscreen = fullscreen;
}

void requestEndlessModeToggle();

bool GameController::isEndlessMode() const {
    return m_endlessMode;
}

void GameController::setEndlessModeDirect(bool endless) {
    m_endlessMode = endless;
}

void GameController::setEndlessMode(bool endless) {
    if (m_endlessMode != endless) {
        requestEndlessModeToggle();
    }
}

void GameController::toggleEndlessMode() {
    requestEndlessModeToggle();
}

void GameController::restart() {
    auto& player = PlayerController::instance();
    if (player.isValid()) {
        uintptr_t base = reinterpret_cast<uintptr_t>(player.getRawInstance());
        player.setY(0);
        player.setSpeed(0.0f);
        player.setMoving(false);
        player.resetDefeat();
        player.setCurrentFrame(0);
        player.setSpeedScale(5.0f);
        player.setSpeedDecrease(0.5f);
        player.setLastMove(PlayerMove::Undefined);
        lowlevel::write<bool>(base + lowlevel::OFFSET_PLAYER_FINAL_ANIM_DONE, false);
    }
}

void GameController::killAndReload() {
    auto& player = PlayerController::instance();
    if (player.isValid()) {
        player.kill();
    }
}

void GameController::quit() {
    ExitProcess(0);
}

uintptr_t GameController::getBaseAddress() const {
    return m_baseAddress;
}

void GameController::update(float deltaTime) {
    m_deltaTime = deltaTime;
    if (deltaTime > 0.0001f) {
        float currentFps = 1.0f / deltaTime;
        m_fps = m_fps * 0.9f + currentFps * 0.1f;
    }
}

void GameController::registerFrame() {
    m_frameCount++;
}

} // namespace sense

