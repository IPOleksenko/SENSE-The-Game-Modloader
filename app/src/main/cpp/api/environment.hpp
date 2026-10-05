#pragma once

#include "types.hpp"
#include <vector>

namespace sense {

enum class WeatherType {
    None,
    Rain,
    Snow,
    Fog,
    Ash,
    GlitchMatrix
};

struct WeatherDrop {
    float x = 0.0f;
    float y = 0.0f;
    float speed = 0.0f;
    float length = 0.0f;
    float alpha = 1.0f;
    float wobble = 0.0f;
};

class EnvironmentController {
public:
    static EnvironmentController& instance();

    // 1. Weather Systems
    void setWeather(WeatherType type);
    WeatherType getWeather() const;
    void setWeatherIntensity(float intensity);
    float getWeatherIntensity() const;
    void setWindAngle(float degrees);
    float getWindAngle() const;

    // 2. Day / Night Cycle & Ambient Lighting
    void setTimeOfDay(float time); // 0.0 (midnight) to 1.0 (24h)
    float getTimeOfDay() const;
    void setCycleEnabled(bool enabled, float dayDurationSeconds = 120.0f);
    bool isCycleEnabled() const;
    Color getAmbientLightColor() const;

    // 3. Lightning / Flash Effect
    void flashLightning(float duration = 0.35f, const Color& color = Color::White());

    // 4. Engine Lifecycle
    void update(float deltaTime, int screenW, int screenH);
    void renderWeather(SDL_Renderer* renderer, int screenW, int screenH);
    void renderLighting(SDL_Renderer* renderer, int screenW, int screenH);

private:
    EnvironmentController();

    WeatherType m_weather = WeatherType::None;
    float m_intensity = 0.8f;
    float m_windAngle = 10.0f;
    std::vector<WeatherDrop> m_drops;

    float m_timeOfDay = 0.5f; // Starts at noon
    bool m_cycleEnabled = false;
    float m_dayDuration = 120.0f;

    float m_lightningTimer = 0.0f;
    float m_lightningDuration = 0.0f;
    Color m_lightningColor = Color::White();
};

// Convenience namespace functions
namespace environment {
    inline void setWeather(WeatherType t) { EnvironmentController::instance().setWeather(t); }
    inline WeatherType getWeather() { return EnvironmentController::instance().getWeather(); }
    inline void setIntensity(float i) { EnvironmentController::instance().setWeatherIntensity(i); }
    inline void setWindAngle(float a) { EnvironmentController::instance().setWindAngle(a); }

    inline void setTimeOfDay(float t) { EnvironmentController::instance().setTimeOfDay(t); }
    inline float getTimeOfDay() { return EnvironmentController::instance().getTimeOfDay(); }
    inline void setCycleEnabled(bool en, float dur = 120.0f) { EnvironmentController::instance().setCycleEnabled(en, dur); }
    inline void flashLightning(float dur = 0.35f, const Color& c = Color::White()) {
        EnvironmentController::instance().flashLightning(dur, c);
    }
}

} // namespace sense

