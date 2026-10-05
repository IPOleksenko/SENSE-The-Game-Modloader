#pragma once

#include "types.hpp"

namespace sense {

class PlayerController {
public:
    static PlayerController& instance();

    bool isValid() const;
    void setRawInstance(void* playerInstance);
    void* getRawInstance() const;

    // Position & Coordinates
    int getY() const;
    void setY(int y);
    void teleport(int y);
    void teleportToCheckpoint(CheckPoint checkpoint);

    // Speed & Balance Mechanics
    float getSpeed() const;
    void setSpeed(float speed);
    float getSpeedMin() const;
    float getSpeedMax() const;
    float getSpeedNormal() const;

    float getSpeedScale() const;
    void setSpeedScale(float scale);
    float getSpeedDecrease() const;
    void setSpeedDecrease(float decrease);

    // Balance Metrics: Normalized from -1.0 (min) to +1.0 (max), with 0.0 being ideal
    float getBalance() const;
    bool isBalanced() const;

    // Movement & States
    PlayerMove getLastMove() const;
    void setLastMove(PlayerMove move);
    bool isMoving() const;
    void setMoving(bool isMoving);

    // Game Over & Loss Handling
    bool hasLost() const;
    void resetDefeat();
    void kill();

    // Animation & Frames
    int getCurrentFrame() const;
    void setCurrentFrame(int frame);
    int getTotalFrames() const;
    bool isFinalAnimationFinished() const;

    // Cheats & Automations
    void setGodMode(bool enabled);
    bool isGodMode() const;

    void setAutoBalance(bool enabled);
    bool isAutoBalance() const;

    void freeze(bool freeze);
    bool isFrozen() const;

    // Perform an in-game simulated step
    void step(PlayerMove direction);

    // Internal tick to enforce god mode and auto-balance
    void update(float deltaTime);

private:
    PlayerController();
    void* m_player = nullptr;
    bool m_godMode = false;
    bool m_autoBalance = false;
    bool m_frozen = false;
    float m_frozenSpeed = 0.0f;
    int m_frozenY = 0;
};

// Convenience namespace functions
namespace player {
    inline bool isValid() { return PlayerController::instance().isValid(); }
    inline int getY() { return PlayerController::instance().getY(); }
    inline void setY(int y) { PlayerController::instance().setY(y); }
    inline void teleport(int y) { PlayerController::instance().teleport(y); }
    inline void teleportToCheckpoint(CheckPoint cp) { PlayerController::instance().teleportToCheckpoint(cp); }

    inline float getSpeed() { return PlayerController::instance().getSpeed(); }
    inline void setSpeed(float s) { PlayerController::instance().setSpeed(s); }
    inline float getSpeedMin() { return PlayerController::instance().getSpeedMin(); }
    inline float getSpeedMax() { return PlayerController::instance().getSpeedMax(); }
    inline float getSpeedNormal() { return PlayerController::instance().getSpeedNormal(); }

    inline float getBalance() { return PlayerController::instance().getBalance(); }
    inline bool isBalanced() { return PlayerController::instance().isBalanced(); }

    inline PlayerMove getLastMove() { return PlayerController::instance().getLastMove(); }
    inline bool isMoving() { return PlayerController::instance().isMoving(); }

    inline bool hasLost() { return PlayerController::instance().hasLost(); }
    inline void resetDefeat() { PlayerController::instance().resetDefeat(); }
    inline void kill() { PlayerController::instance().kill(); }

    inline void setGodMode(bool enable) { PlayerController::instance().setGodMode(enable); }
    inline bool isGodMode() { return PlayerController::instance().isGodMode(); }

    inline void setAutoBalance(bool enable) { PlayerController::instance().setAutoBalance(enable); }
    inline bool isAutoBalance() { return PlayerController::instance().isAutoBalance(); }

    inline void freeze(bool enable) { PlayerController::instance().freeze(enable); }
    inline bool isFrozen() { return PlayerController::instance().isFrozen(); }
}

} // namespace sense

