#include "physics.hpp"
#include "player.hpp"
#include "render.hpp"
#include <cmath>
#include <algorithm>

namespace sense {

PhysicsController& PhysicsController::instance() {
    static PhysicsController inst;
    return inst;
}

void PhysicsController::setFrictionMultiplier(float multiplier) {
    m_frictionMultiplier = std::max(0.0f, multiplier);
}

float PhysicsController::getFrictionMultiplier() const {
    return m_frictionMultiplier;
}

void PhysicsController::setAccelerationMultiplier(float multiplier) {
    m_accelMultiplier = std::max(0.0f, multiplier);
}

float PhysicsController::getAccelerationMultiplier() const {
    return m_accelMultiplier;
}

void PhysicsController::setBalanceSensitivity(float multiplier) {
    m_balanceSensitivity = std::max(0.0f, multiplier);
}

float PhysicsController::getBalanceSensitivity() const {
    return m_balanceSensitivity;
}

void PhysicsController::setWindForce(float force) {
    m_windForce = force;
}

float PhysicsController::getWindForce() const {
    return m_windForce;
}

void PhysicsController::setWindTurbulence(bool enabled) {
    m_windTurbulence = enabled;
}

bool PhysicsController::hasWindTurbulence() const {
    return m_windTurbulence;
}

void PhysicsController::setCustomSpeedLimits(bool enabled, float minSpeed, float maxSpeed) {
    m_customLimitsEnabled = enabled;
    m_customMinSpeed = minSpeed;
    m_customMaxSpeed = maxSpeed;
}

bool PhysicsController::hasCustomSpeedLimits() const {
    return m_customLimitsEnabled;
}

float PhysicsController::getCustomMinSpeed() const {
    return m_customMinSpeed;
}

float PhysicsController::getCustomMaxSpeed() const {
    return m_customMaxSpeed;
}

void PhysicsController::applyPreset(PhysicsPreset preset) {
    switch (preset) {
        case PhysicsPreset::Default:
            reset();
            RenderController::instance().showNotification("Physics: Default Settings", 1.5f);
            break;
        case PhysicsPreset::MoonGravity:
            m_frictionMultiplier = 0.25f;
            m_accelMultiplier = 0.8f;
            m_windForce = 0.0f;
            m_windTurbulence = false;
            m_customLimitsEnabled = true;
            m_customMinSpeed = 10.0f;
            m_customMaxSpeed = 200.0f;
            RenderController::instance().showNotification("Physics: Moon Gravity (Low Friction)", 2.0f, Color::Cyan());
            break;
        case PhysicsPreset::SupercarTraction:
            m_frictionMultiplier = 1.5f;
            m_accelMultiplier = 2.5f;
            m_windForce = 0.0f;
            m_windTurbulence = false;
            m_customLimitsEnabled = true;
            m_customMinSpeed = 40.0f;
            m_customMaxSpeed = 350.0f;
            RenderController::instance().showNotification("Physics: Supercar Traction (Nitrous Boost)", 2.0f, Color::Green());
            break;
        case PhysicsPreset::IceRoad:
            m_frictionMultiplier = 0.05f;
            m_accelMultiplier = 1.2f;
            m_windForce = 5.0f;
            m_windTurbulence = true;
            RenderController::instance().showNotification("Physics: Ice Road Drift (Slippery)", 2.0f, Color::Cyan());
            break;
        case PhysicsPreset::StormyHeadwind:
            m_frictionMultiplier = 1.8f;
            m_accelMultiplier = 1.0f;
            m_windForce = -15.0f;
            m_windTurbulence = true;
            RenderController::instance().showNotification("Physics: Stormy Headwind (-15 force)", 2.0f, Color::Yellow());
            break;
    }
}

void PhysicsController::reset() {
    m_frictionMultiplier = 1.0f;
    m_accelMultiplier = 1.0f;
    m_balanceSensitivity = 1.0f;
    m_windForce = 0.0f;
    m_windTurbulence = false;
    m_customLimitsEnabled = false;
    m_customMinSpeed = 50.5f;
    m_customMaxSpeed = 150.0f;
}

void PhysicsController::update(float deltaTime) {
    auto& pc = PlayerController::instance();
    if (!pc.isValid() || !pc.isMoving()) return;

    // Apply wind and turbulence
    m_turbulenceTimer += deltaTime;
    float gust = 0.0f;
    if (m_windTurbulence) {
        gust = std::sin(m_turbulenceTimer * 3.0f) * 4.0f + std::cos(m_turbulenceTimer * 1.3f) * 2.0f;
    }
    float effectiveWind = m_windForce + gust;

    if (std::abs(effectiveWind) > 0.01f) {
        float currentSpeed = pc.getSpeed();
        float newSpeed = currentSpeed + (effectiveWind * deltaTime);
        pc.setSpeed(std::max(0.0f, newSpeed));
    }

    // Apply custom speed limits immunity
    if (m_customLimitsEnabled) {
        float speed = pc.getSpeed();
        if (speed >= pc.getSpeedMin() && speed <= m_customMaxSpeed && pc.hasLost()) {
            pc.resetDefeat();
        } else if (speed >= m_customMinSpeed && speed <= pc.getSpeedMax() && pc.hasLost()) {
            pc.resetDefeat();
        }
    }
}

} // namespace sense

