#include "particles.hpp"
#include "render.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace sense {

static float randFloat(float minVal, float maxVal) {
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

ParticleController::ParticleController() {
    m_particles.reserve(500);
}

ParticleController& ParticleController::instance() {
    static ParticleController inst;
    return inst;
}

void ParticleController::spawnParticle(const Particle& p) {
    if (m_particles.size() < m_maxParticles) {
        m_particles.push_back(p);
    }
}

void ParticleController::spawnSparks(float x, float y, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = { x, y };
        float angle = randFloat(-3.14159f * 0.8f, -3.14159f * 0.2f);
        float speed = randFloat(120.0f, 350.0f);
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.acc = { 0.0f, 400.0f }; // gravity
        p.colorStart = color;
        p.colorEnd = Color(255, 60, 0, 0); // fade to transparent red/orange
        p.sizeStart = randFloat(2.5f, 4.5f);
        p.sizeEnd = 1.0f;
        p.lifetime = randFloat(0.25f, 0.6f);
        p.screenSpace = true;
        spawnParticle(p);
    }
}

void ParticleController::spawnSpeedLines(int screenW, int screenH, float intensity) {
    int count = static_cast<int>(5 * intensity);
    for (int i = 0; i < count; ++i) {
        Particle p;
        bool leftSide = (rand() % 2 == 0);
        float x = leftSide ? randFloat(20.0f, 150.0f) : randFloat(screenW - 150.0f, screenW - 20.0f);
        float y = randFloat(0.0f, static_cast<float>(screenH));
        p.pos = { x, y };
        p.vel = { 0.0f, randFloat(600.0f, 1200.0f) };
        p.acc = { 0.0f, 0.0f };
        p.colorStart = Color(200, 240, 255, static_cast<uint8_t>(180 * intensity));
        p.colorEnd = Color(200, 240, 255, 0);
        p.sizeStart = randFloat(2.0f, 3.5f);
        p.sizeEnd = p.sizeStart;
        p.lifetime = randFloat(0.1f, 0.25f);
        p.screenSpace = true;
        spawnParticle(p);
    }
}

void ParticleController::spawnSmoke(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = { x + randFloat(-5.0f, 5.0f), y + randFloat(-5.0f, 5.0f) };
        p.vel = { randFloat(-20.0f, 20.0f), randFloat(30.0f, 80.0f) };
        p.acc = { 0.0f, -10.0f };
        p.colorStart = Color(180, 180, 190, 160);
        p.colorEnd = Color(120, 120, 130, 0);
        p.sizeStart = randFloat(4.0f, 6.0f);
        p.sizeEnd = randFloat(12.0f, 20.0f);
        p.lifetime = randFloat(0.4f, 0.9f);
        p.screenSpace = true;
        spawnParticle(p);
    }
}

void ParticleController::spawnBurst(float x, float y, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = { x, y };
        float angle = randFloat(0.0f, 6.28318f);
        float speed = randFloat(80.0f, 280.0f);
        p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
        p.acc = { 0.0f, 50.0f };
        p.colorStart = color;
        p.colorEnd = Color(color.r, color.g, color.b, 0);
        p.sizeStart = randFloat(3.0f, 6.0f);
        p.sizeEnd = 0.5f;
        p.lifetime = randFloat(0.4f, 0.8f);
        p.screenSpace = true;
        spawnParticle(p);
    }
}

void ParticleController::spawnTrail(float x, float y, Color color) {
    Particle p;
    p.pos = { x, y };
    p.vel = { randFloat(-10.0f, 10.0f), randFloat(40.0f, 80.0f) };
    p.acc = { 0.0f, 0.0f };
    p.colorStart = color;
    p.colorEnd = Color(color.r, color.g, color.b, 0);
    p.sizeStart = 5.0f;
    p.sizeEnd = 1.0f;
    p.lifetime = 0.35f;
    p.screenSpace = true;
    spawnParticle(p);
}

void ParticleController::update(float deltaTime) {
    for (auto& p : m_particles) {
        p.pos.x += p.vel.x * deltaTime;
        p.pos.y += p.vel.y * deltaTime;
        p.vel.x += p.acc.x * deltaTime;
        p.vel.y += p.acc.y * deltaTime;
        p.age += deltaTime;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle& p) { return p.age >= p.lifetime; }),
        m_particles.end()
    );
}

void ParticleController::render(SDL_Renderer* renderer) {
    if (!renderer || m_particles.empty()) return;

    for (const auto& p : m_particles) {
        float t = p.age / p.lifetime;
        if (t > 1.0f) t = 1.0f;

        // Lerp color & alpha
        uint8_t r = static_cast<uint8_t>(p.colorStart.r + t * (p.colorEnd.r - p.colorStart.r));
        uint8_t g = static_cast<uint8_t>(p.colorStart.g + t * (p.colorEnd.g - p.colorStart.g));
        uint8_t b = static_cast<uint8_t>(p.colorStart.b + t * (p.colorEnd.b - p.colorStart.b));
        uint8_t a = static_cast<uint8_t>(p.colorStart.a + t * (p.colorEnd.a - p.colorStart.a));
        Color c(r, g, b, a);

        float size = p.sizeStart + t * (p.sizeEnd - p.sizeStart);
        int isize = static_cast<int>(size);
        if (isize < 1) isize = 1;

        Rect rc(static_cast<int>(p.pos.x - size * 0.5f), static_cast<int>(p.pos.y - size * 0.5f), isize, isize);
        RenderController::instance().fillRect(renderer, rc, c);
    }
}

void ParticleController::clear() {
    m_particles.clear();
}

size_t ParticleController::getActiveParticleCount() const {
    return m_particles.size();
}

} // namespace sense

