#pragma once

#include "types.hpp"

namespace sense {

enum class PhysicsPreset {
    Default,
    MoonGravity,
    SupercarTraction,
    IceRoad,
    StormyHeadwind
};

class PhysicsController {
public:
    static PhysicsController& instance();

    // 1. Multipliers & Modifiers
    void setFrictionMultiplier(float multiplier);
    float getFrictionMultiplier() const;

    void setAccelerationMultiplier(float multiplier);
    float getAccelerationMultiplier() const;

    void setBalanceSensitivity(float multiplier);
    float getBalanceSensitivity() const;

    // 2. Wind & Aerodynamic Forces
    void setWindForce(float force);
    float getWindForce() const;
    void setWindTurbulence(bool enabled);
    bool hasWindTurbulence() const;

    // 3. Custom Speed Limits
    void setCustomSpeedLimits(bool enabled, float minSpeed = 30.0f, float maxSpeed = 300.0f);
    bool hasCustomSpeedLimits() const;
    float getCustomMinSpeed() const;
    float getCustomMaxSpeed() const;

    // 4. Presets
    void applyPreset(PhysicsPreset preset);
    void reset();

    // Internal tick to apply modifiers to Player
    void update(float deltaTime);

private:
    PhysicsController() = default;

    float m_frictionMultiplier = 1.0f;
    float m_accelMultiplier = 1.0f;
    float m_balanceSensitivity = 1.0f;

    float m_windForce = 0.0f;
    bool m_windTurbulence = false;
    float m_turbulenceTimer = 0.0f;
    float m_currentTurbulence = 0.0f;

    bool m_customLimitsEnabled = false;
    float m_customMinSpeed = 50.5f;
    float m_customMaxSpeed = 150.0f;
};

// Convenience namespace functions
namespace physics {
    inline void setFriction(float m) { PhysicsController::instance().setFrictionMultiplier(m); }
    inline float getFriction() { return PhysicsController::instance().getFrictionMultiplier(); }

    inline void setAcceleration(float m) { PhysicsController::instance().setAccelerationMultiplier(m); }
    inline float getAcceleration() { return PhysicsController::instance().getAccelerationMultiplier(); }

    inline void setWind(float force) { PhysicsController::instance().setWindForce(force); }
    inline void setTurbulence(bool en) { PhysicsController::instance().setWindTurbulence(en); }

    inline void setSpeedLimits(bool en, float minS = 30.0f, float maxS = 300.0f) {
        PhysicsController::instance().setCustomSpeedLimits(en, minS, maxS);
    }

    inline void applyPreset(PhysicsPreset p) { PhysicsController::instance().applyPreset(p); }
    inline void reset() { PhysicsController::instance().reset(); }
}

} // namespace sense

