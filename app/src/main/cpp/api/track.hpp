#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>

namespace sense {

enum class TrackZoneType {
    BoostPad,
    OilSlick,
    Slowdown,
    LowGravity,
    Hazard,
    Custom
};

struct TrackZone {
    int id = 0;
    std::string name;
    int startY = 0;
    int endY = 0;
    TrackZoneType type = TrackZoneType::BoostPad;
    Color color = Color::Yellow();
    std::function<void(float dt)> onInside;
};

class TrackController {
public:
    static TrackController& instance();

    int addZone(
        int startY,
        int endY,
        TrackZoneType type,
        const std::string& name = "",
        const Color& color = Color::Yellow(),
        std::function<void(float dt)> onInside = nullptr
    );

    void removeZone(int id);
    void clearZones();
    const std::vector<TrackZone>& getZones() const { return m_zones; }

    void update(float deltaTime);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    TrackController();

    int m_nextId = 1;
    std::vector<TrackZone> m_zones;
    int m_activeZoneId = -1;
};

namespace track {
    inline int addZone(int startY, int endY, TrackZoneType type, const std::string& name = "", const Color& color = Color::Yellow(), std::function<void(float)> onInside = nullptr) {
        return TrackController::instance().addZone(startY, endY, type, name, color, onInside);
    }
    inline void removeZone(int id) { TrackController::instance().removeZone(id); }
    inline void clearZones() { TrackController::instance().clearZones(); }
}

} // namespace sense

