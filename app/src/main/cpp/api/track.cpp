#include "track.hpp"
#include "player.hpp"
#include "render.hpp"
#include "particles.hpp"
#include <algorithm>

namespace sense {

TrackController& TrackController::instance() {
    static TrackController inst;
    return inst;
}

TrackController::TrackController() = default;

int TrackController::addZone(
    int startY,
    int endY,
    TrackZoneType type,
    const std::string& name,
    const Color& color,
    std::function<void(float dt)> onInside
) {
    TrackZone z;
    z.id = m_nextId++;
    z.startY = startY;
    z.endY = endY;
    z.type = type;
    z.name = name.empty() ? "TrackZone" : name;
    z.color = color;
    z.onInside = onInside;
    m_zones.push_back(z);
    return z.id;
}

void TrackController::removeZone(int id) {
    m_zones.erase(
        std::remove_if(m_zones.begin(), m_zones.end(), [id](const TrackZone& z) { return z.id == id; }),
        m_zones.end()
    );
}

void TrackController::clearZones() {
    m_zones.clear();
}

void TrackController::update(float deltaTime) {
    auto& p = PlayerController::instance();
    if (!p.isValid()) return;

    int curY = p.getY();
    int hitZoneId = -1;

    for (const auto& z : m_zones) {
        if (curY >= z.startY && curY <= z.endY) {
            hitZoneId = z.id;

            if (z.type == TrackZoneType::BoostPad) {
                float curSpd = p.getSpeed();
                p.setSpeed((std::min)(150.0f, curSpd + 60.0f * deltaTime));
                ParticleController::instance().spawnSparks(640, 500, 3, Color::Cyan());
            } else if (z.type == TrackZoneType::OilSlick) {
                float curSpd = p.getSpeed();
                p.setSpeed((std::max)(50.5f, curSpd - 30.0f * deltaTime));
                ParticleController::instance().spawnSmoke(640, 500, 2);
            } else if (z.type == TrackZoneType::Slowdown) {
                float curSpd = p.getSpeed();
                p.setSpeed((std::max)(50.5f, curSpd - 45.0f * deltaTime));
            }

            if (z.onInside) {
                z.onInside(deltaTime);
            }
        }
    }

    if (hitZoneId != m_activeZoneId) {
        m_activeZoneId = hitZoneId;
        if (m_activeZoneId != -1) {
            for (const auto& z : m_zones) {
                if (z.id == m_activeZoneId) {
                    RenderController::instance().showNotification("Entered Zone: " + z.name, 1.5f, z.color);
                    break;
                }
            }
        }
    }
}

void TrackController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!renderer || m_zones.empty()) return;

    auto& p = PlayerController::instance();
    if (!p.isValid()) return;

    int curY = p.getY();
    auto& rc = RenderController::instance();

    // Render road zone markers for nearby zones (within 1000px ahead/behind)
    for (const auto& z : m_zones) {
        int dist = z.startY - curY;
        if (dist >= -200 && dist <= 1000) {
            // Projected screen Y position (road moves towards screen bottom)
            // Near = lower on screen (y = 550), Far = higher on screen (y = 200)
            float norm = 1.0f - (static_cast<float>(dist) / 1000.0f);
            int markerY = 220 + static_cast<int>(norm * 320.0f);
            int markerW = 200 + static_cast<int>(norm * 180.0f);
            int markerX = (screenW - markerW) / 2;

            rc.fillRect(renderer, Rect(markerX, markerY, markerW, 14), Color(z.color.r, z.color.g, z.color.b, 160));
            rc.drawRect(renderer, Rect(markerX, markerY, markerW, 14), Color::White());
            rc.drawText(renderer, z.name, markerX + 10, markerY - 14, z.color, 0.75f);
        }
    }
}

} // namespace sense

