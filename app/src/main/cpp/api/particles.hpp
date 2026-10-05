#pragma once

#include "types.hpp"
#include <vector>

namespace sense {

struct Particle {
    PointF pos;
    PointF vel;
    PointF acc;
    Color colorStart;
    Color colorEnd;
    float sizeStart = 4.0f;
    float sizeEnd = 1.0f;
    float lifetime = 1.0f;
    float age = 0.0f;
    bool screenSpace = true;
};

class ParticleController {
public:
    static ParticleController& instance();

    // 1. Spawning Presets
    void spawnSparks(float x, float y, int count = 15, Color color = Color::Yellow());
    void spawnSpeedLines(int screenW, int screenH, float intensity = 1.0f);
    void spawnSmoke(float x, float y, int count = 6);
    void spawnBurst(float x, float y, int count = 30, Color color = Color::Cyan());
    void spawnTrail(float x, float y, Color color = Color(100, 200, 255, 180));

    // 2. Custom Particle Spawner
    void spawnParticle(const Particle& p);

    // 3. Engine Lifecycle
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    void clear();

    size_t getActiveParticleCount() const;

private:
    ParticleController();

    std::vector<Particle> m_particles;
    size_t m_maxParticles = 2000;
};

// Convenience namespace functions
namespace particles {
    inline void spawnSparks(float x, float y, int count = 15, Color c = Color::Yellow()) {
        ParticleController::instance().spawnSparks(x, y, count, c);
    }
    inline void spawnSpeedLines(int screenW, int screenH, float intensity = 1.0f) {
        ParticleController::instance().spawnSpeedLines(screenW, screenH, intensity);
    }
    inline void spawnSmoke(float x, float y, int count = 6) {
        ParticleController::instance().spawnSmoke(x, y, count);
    }
    inline void spawnBurst(float x, float y, int count = 30, Color c = Color::Cyan()) {
        ParticleController::instance().spawnBurst(x, y, count, c);
    }
    inline void spawnTrail(float x, float y, Color c = Color(100, 200, 255, 180)) {
        ParticleController::instance().spawnTrail(x, y, c);
    }
    inline void clear() {
        ParticleController::instance().clear();
    }
}

} // namespace sense

