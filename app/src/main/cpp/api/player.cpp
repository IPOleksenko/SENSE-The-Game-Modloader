#include "player.hpp"
#include "lowlevel.hpp"
#include <cmath>

namespace sense {

PlayerController::PlayerController() = default;

PlayerController& PlayerController::instance() {
    static PlayerController inst;
    return inst;
}

bool PlayerController::isValid() const {
    if (!m_player) return false;
    uintptr_t base = reinterpret_cast<uintptr_t>(m_player);

    float speedMin = lowlevel::read<float>(base + lowlevel::OFFSET_PLAYER_SPEED_MIN);
    int framesTotal = lowlevel::read<int>(base + lowlevel::OFFSET_PLAYER_FRAMES_TOTAL);

    return (std::abs(speedMin - 50.5f) < 0.01f && framesTotal == 120);
}

void PlayerController::setRawInstance(void* playerInstance) {
    m_player = playerInstance;
    lowlevel::setRawPlayer(playerInstance);
}

void* PlayerController::getRawInstance() const {
    return m_player;
}

int PlayerController::getY() const {
    if (!isValid()) return 0;
    return lowlevel::read<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_POS_Y);
}

void PlayerController::setY(int y) {
    if (!isValid()) return;
    lowlevel::write<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_POS_Y, y);
}

void PlayerController::teleport(int y) {
    setY(y);
}

void PlayerController::teleportToCheckpoint(CheckPoint cp) {
    teleport(static_cast<int>(cp));
}

float PlayerController::getSpeed() const {
    if (!isValid()) return 0.0f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED);
}

void PlayerController::setSpeed(float speed) {
    if (!isValid()) return;
    lowlevel::write<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED, speed);
}

float PlayerController::getSpeedMin() const {
    if (!isValid()) return 50.5f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_MIN);
}

float PlayerController::getSpeedMax() const {
    if (!isValid()) return 150.0f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_MAX);
}

float PlayerController::getSpeedNormal() const {
    if (!isValid()) return 100.25f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_NORMAL);
}

float PlayerController::getSpeedScale() const {
    if (!isValid()) return 5.0f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_SCALE);
}

void PlayerController::setSpeedScale(float scale) {
    if (!isValid()) return;
    lowlevel::write<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_SCALE, scale);
}

float PlayerController::getSpeedDecrease() const {
    if (!isValid()) return 0.5f;
    return lowlevel::read<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_DECREASE);
}

void PlayerController::setSpeedDecrease(float decrease) {
    if (!isValid()) return;
    lowlevel::write<float>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_SPEED_DECREASE, decrease);
}

float PlayerController::getBalance() const {
    if (!isValid()) return 0.0f;
    float spd = getSpeed();
    float sMin = getSpeedMin();
    float sMax = getSpeedMax();
    float sNormal = getSpeedNormal();

    if (spd < sNormal) {
        return (spd - sNormal) / (sNormal - sMin);
    } else {
        return (spd - sNormal) / (sMax - sNormal);
    }
}

bool PlayerController::isBalanced() const {
    return std::abs(getBalance()) < 0.85f;
}

PlayerMove PlayerController::getLastMove() const {
    if (!isValid()) return PlayerMove::Undefined;
    int move = lowlevel::read<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_LAST_MOVE);
    return static_cast<PlayerMove>(move);
}

void PlayerController::setLastMove(PlayerMove move) {
    if (!isValid()) return;
    lowlevel::write<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_LAST_MOVE, static_cast<int>(move));
}

bool PlayerController::isMoving() const {
    if (!isValid()) return false;
    return lowlevel::read<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_IS_MOVING);
}

void PlayerController::setMoving(bool isMoving) {
    if (!isValid()) return;
    lowlevel::write<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_IS_MOVING, isMoving);
}

bool PlayerController::hasLost() const {
    if (!isValid()) return false;
    return lowlevel::read<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_HAS_LOST);
}

void PlayerController::resetDefeat() {
    if (!isValid()) return;
    lowlevel::write<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_HAS_LOST, false);
}

void PlayerController::kill() {
    if (!isValid()) return;
    lowlevel::write<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_HAS_LOST, true);
}

int PlayerController::getCurrentFrame() const {
    if (!isValid()) return 0;
    return lowlevel::read<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_FRAME_CURRENT);
}

void PlayerController::setCurrentFrame(int frame) {
    if (!isValid()) return;
    lowlevel::write<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_FRAME_CURRENT, frame);
}

int PlayerController::getTotalFrames() const {
    if (!isValid()) return 120;
    return lowlevel::read<int>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_FRAMES_TOTAL);
}

bool PlayerController::isFinalAnimationFinished() const {
    if (!isValid()) return false;
    return lowlevel::read<bool>(reinterpret_cast<uintptr_t>(m_player) + lowlevel::OFFSET_PLAYER_FINAL_ANIM_DONE);
}

void PlayerController::setGodMode(bool enabled) {
    m_godMode = enabled;
    if (m_godMode) {
        resetDefeat();
    }
}

bool PlayerController::isGodMode() const {
    return m_godMode;
}

void PlayerController::setAutoBalance(bool enabled) {
    m_autoBalance = enabled;
}

bool PlayerController::isAutoBalance() const {
    return m_autoBalance;
}

void PlayerController::freeze(bool freeze) {
    m_frozen = freeze;
    if (m_frozen && isValid()) {
        m_frozenSpeed = getSpeed();
        m_frozenY = getY();
    }
}

bool PlayerController::isFrozen() const {
    return m_frozen;
}

void PlayerController::step(PlayerMove direction) {
    if (!isValid() || direction == PlayerMove::Undefined) return;
    if (getLastMove() != direction) {
        setLastMove(direction);
        setSpeed(getSpeed() + getSpeedScale());
    }
}

void PlayerController::update(float deltaTime) {
    if (!isValid()) return;

    if (m_godMode) {
        resetDefeat();
        if (isMoving()) {
            float currentSpeed = getSpeed();
            float minSpd = getSpeedMin() + 2.0f;
            float maxSpd = getSpeedMax() - 2.0f;

            if (currentSpeed < minSpd) {
                setSpeed(getSpeedNormal());
            } else if (currentSpeed > maxSpd) {
                setSpeed(getSpeedNormal());
            }
        }
    }

    if (m_autoBalance) {
        if (isMoving()) {
            float currentSpeed = getSpeed();
            float targetSpeed = getSpeedNormal();
            float corrected = currentSpeed + (targetSpeed - currentSpeed) * 0.25f;
            setSpeed(corrected);
        }
    }

    if (m_frozen) {
        setY(m_frozenY);
        setSpeed(m_frozenSpeed);
    }
}

} // namespace sense

