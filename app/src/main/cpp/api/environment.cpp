#include "environment.hpp"
#include "render.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace sense {

static float randF(float minV, float maxV) {
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minV + r * (maxV - minV);
}

EnvironmentController::EnvironmentController() {
    m_drops.reserve(300);
}

EnvironmentController& EnvironmentController::instance() {
    static EnvironmentController inst;
    return inst;
}

void EnvironmentController::setWeather(WeatherType type) {
    m_weather = type;
    m_drops.clear();
}

WeatherType EnvironmentController::getWeather() const {
    return m_weather;
}

void EnvironmentController::setWeatherIntensity(float intensity) {
    m_intensity = std::clamp(intensity, 0.0f, 1.0f);
}

float EnvironmentController::getWeatherIntensity() const {
    return m_intensity;
}

void EnvironmentController::setWindAngle(float degrees) {
    m_windAngle = degrees;
}

float EnvironmentController::getWindAngle() const {
    return m_windAngle;
}

void EnvironmentController::setTimeOfDay(float time) {
    m_timeOfDay = std::fmod(time, 1.0f);
    if (m_timeOfDay < 0.0f) m_timeOfDay += 1.0f;
}

float EnvironmentController::getTimeOfDay() const {
    return m_timeOfDay;
}

void EnvironmentController::setCycleEnabled(bool enabled, float dayDurationSeconds) {
    m_cycleEnabled = enabled;
    m_dayDuration = std::max(5.0f, dayDurationSeconds);
}

bool EnvironmentController::isCycleEnabled() const {
    return m_cycleEnabled;
}

Color EnvironmentController::getAmbientLightColor() const {
    // 0.0 = midnight (dark navy blue)
    // 0.25 = sunrise (warm golden orange)
    // 0.50 = noon (clear white/no darkness)
    // 0.75 = sunset (deep purple/crimson)
    float t = m_timeOfDay;

    if (t < 0.25f) {
        // Midnight -> Sunrise
        float f = t / 0.25f;
        uint8_t a = static_cast<uint8_t>(170 * (1.0f - f) + 70 * f);
        return Color(static_cast<uint8_t>(15 + 160 * f), static_cast<uint8_t>(20 + 80 * f), static_cast<uint8_t>(60 - 20 * f), a);
    } else if (t < 0.50f) {
        // Sunrise -> Noon
        float f = (t - 0.25f) / 0.25f;
        uint8_t a = static_cast<uint8_t>(70 * (1.0f - f));
        return Color(255, static_cast<uint8_t>(150 + 105 * f), static_cast<uint8_t>(50 + 205 * f), a);
    } else if (t < 0.75f) {
        // Noon -> Sunset
        float f = (t - 0.50f) / 0.25f;
        uint8_t a = static_cast<uint8_t>(80 * f);
        return Color(static_cast<uint8_t>(255 - 60 * f), static_cast<uint8_t>(255 - 180 * f), static_cast<uint8_t>(255 - 100 * f), a);
    } else {
        // Sunset -> Midnight
        float f = (t - 0.75f) / 0.25f;
        uint8_t a = static_cast<uint8_t>(80 * (1.0f - f) + 170 * f);
        return Color(static_cast<uint8_t>(195 - 180 * f), static_cast<uint8_t>(75 - 55 * f), static_cast<uint8_t>(155 - 95 * f), a);
    }
}

void EnvironmentController::flashLightning(float duration, const Color& color) {
    m_lightningDuration = duration;
    m_lightningTimer = duration;
    m_lightningColor = color;
}

void EnvironmentController::update(float deltaTime, int screenW, int screenH) {
    // 1. Day / Night cycle
    if (m_cycleEnabled) {
        m_timeOfDay += (deltaTime / m_dayDuration);
        if (m_timeOfDay >= 1.0f) m_timeOfDay -= 1.0f;
    }

    // 2. Lightning flash timer
    if (m_lightningTimer > 0.0f) {
        m_lightningTimer -= deltaTime;
        if (m_lightningTimer < 0.0f) m_lightningTimer = 0.0f;
    }

    // 3. Weather simulation
    if (m_weather == WeatherType::None) {
        m_drops.clear();
        return;
    }

    size_t targetDrops = static_cast<size_t>(250 * m_intensity);
    if (m_weather == WeatherType::Snow) targetDrops = static_cast<size_t>(120 * m_intensity);
    if (m_weather == WeatherType::Fog) targetDrops = static_cast<size_t>(15 * m_intensity);

    while (m_drops.size() < targetDrops) {
        WeatherDrop d;
        d.x = randF(-100.0f, static_cast<float>(screenW + 100));
        d.y = randF(-50.0f, static_cast<float>(screenH));
        d.speed = (m_weather == WeatherType::Rain) ? randF(700.0f, 1200.0f) :
                  (m_weather == WeatherType::Snow) ? randF(80.0f, 220.0f) :
                  (m_weather == WeatherType::Ash)  ? randF(-150.0f, -40.0f) : randF(30.0f, 60.0f);
        d.length = (m_weather == WeatherType::Rain) ? randF(12.0f, 25.0f) : randF(2.0f, 5.0f);
        d.alpha = randF(0.4f, 0.9f);
        d.wobble = randF(0.0f, 6.28f);
        m_drops.push_back(d);
    }

    float rad = m_windAngle * 3.14159f / 180.0f;
    float windVx = std::sin(rad) * 400.0f;

    for (auto& d : m_drops) {
        d.wobble += deltaTime * 3.0f;
        float wobbleOffset = (m_weather == WeatherType::Snow) ? std::sin(d.wobble) * 30.0f : 0.0f;

        d.x += (windVx + wobbleOffset) * deltaTime;
        d.y += d.speed * deltaTime;

        if (d.y > screenH + 20) {
            d.y = -20.0f;
            d.x = randF(-50.0f, static_cast<float>(screenW + 50));
        } else if (d.y < -30.0f && m_weather == WeatherType::Ash) {
            d.y = static_cast<float>(screenH + 20);
            d.x = randF(-50.0f, static_cast<float>(screenW + 50));
        }

        if (d.x < -100.0f) d.x = static_cast<float>(screenW + 50);
        if (d.x > screenW + 100.0f) d.x = -50.0f;
    }
}

void EnvironmentController::renderWeather(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!renderer || m_weather == WeatherType::None || m_drops.empty()) return;

    if (m_weather == WeatherType::Rain) {
        float rad = m_windAngle * 3.14159f / 180.0f;
        float dx = std::sin(rad);
        for (const auto& d : m_drops) {
            Color c(180, 215, 255, static_cast<uint8_t>(200 * d.alpha));
            int x1 = static_cast<int>(d.x);
            int y1 = static_cast<int>(d.y);
            int x2 = static_cast<int>(d.x + dx * d.length);
            int y2 = static_cast<int>(d.y + d.length);
            RenderController::instance().drawLine(renderer, x1, y1, x2, y2, c);
        }
    } else if (m_weather == WeatherType::Snow) {
        for (const auto& d : m_drops) {
            Color c(245, 250, 255, static_cast<uint8_t>(230 * d.alpha));
            int size = static_cast<int>(d.length);
            RenderController::instance().fillRect(renderer, Rect(static_cast<int>(d.x), static_cast<int>(d.y), size, size), c);
        }
    } else if (m_weather == WeatherType::Ash) {
        for (const auto& d : m_drops) {
            Color c(255, 120, 40, static_cast<uint8_t>(220 * d.alpha));
            RenderController::instance().fillRect(renderer, Rect(static_cast<int>(d.x), static_cast<int>(d.y), 3, 3), c);
        }
    } else if (m_weather == WeatherType::GlitchMatrix) {
        for (const auto& d : m_drops) {
            Color c(50, 255, 100, static_cast<uint8_t>(220 * d.alpha));
            RenderController::instance().fillRect(renderer, Rect(static_cast<int>(d.x), static_cast<int>(d.y), 2, static_cast<int>(d.length * 2)), c);
        }
    }
}

void EnvironmentController::renderLighting(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!renderer) return;

    // Ambient Lighting
    Color ambient = getAmbientLightColor();
    if (ambient.a > 0) {
        RenderController::instance().fillRect(renderer, Rect(0, 0, screenW, screenH), ambient);
    }

    // Lightning Flash
    if (m_lightningTimer > 0.0f && m_lightningDuration > 0.0f) {
        float f = m_lightningTimer / m_lightningDuration;
        Color c = m_lightningColor;
        c.a = static_cast<uint8_t>(240 * f);
        RenderController::instance().fillRect(renderer, Rect(0, 0, screenW, screenH), c);
    }
}

} // namespace sense

