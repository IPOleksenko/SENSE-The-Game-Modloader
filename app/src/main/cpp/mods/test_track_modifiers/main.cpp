#include <api/sense_api.hpp>

class TestTrackModifiersMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Test: Track Modifiers & Hazards API";
        info.version = "1.0.0";
        info.author = "SenseTestSuite";
        info.description = "Test harness for procedural track zones, highway boost pads, oil slick hazards, and road surface physics.";
        return info;
    }

    void onInit() override {
        sense::log("TestTrackModifiersMod: Initializing track zones test harness!");

        spawnDefaultTrackZones();

        // Hotkey '2': Spawn dynamic zones ahead of player
        sense::input::registerHotkey(sense::Key_2, [this]() {
            spawnZonesAheadOfPlayer();
        });

        // F1 Menu Section
        sense::ui::addSection("API Test: Track Zones & Hazards");
        sense::ui::addButton("Spawn Zones Ahead of Player (2)", [this]() {
            spawnZonesAheadOfPlayer();
        });
        sense::ui::addButton("Reset to Default Track Zones", [this]() {
            spawnDefaultTrackZones();
            sense::render::showNotification("Default Track Zones Spawned!", 2.0f);
        });
        sense::ui::addButton("Clear All Track Zones", []() {
            sense::track::clearZones();
            sense::render::showNotification("All Track Zones Cleared", 1.5f);
        });
    }

    void spawnDefaultTrackZones() {
        sense::track::clearZones();

        // Zone 1: Boost Pad at 800 - 1100 px
        sense::track::addZone(800, 1100, sense::TrackZoneType::BoostPad, ">> NITRO BOOST PAD <<", sense::Color::Green());

        // Zone 2: Oil Slick at 2000 - 2300 px
        sense::track::addZone(2000, 2300, sense::TrackZoneType::OilSlick, "! OIL SLICK - REDUCE SPEED !", sense::Color::Yellow());

        // Zone 3: Supercharger at 3500 - 3800 px
        sense::track::addZone(3500, 3800, sense::TrackZoneType::BoostPad, ">> HYPERDRIVE ZONE <<", sense::Color::Cyan());

        // Zone 4: Slowdown Speed Bumps at 5500 - 5800 px
        sense::track::addZone(5500, 5800, sense::TrackZoneType::Slowdown, "- SPEED BUMPS HAZARD -", sense::Color::Red());
    }

    void spawnZonesAheadOfPlayer() {
        auto& p = sense::api().player();
        int curY = p.isValid() ? p.getY() : 0;

        sense::track::clearZones();
        sense::track::addZone(curY + 300, curY + 600, sense::TrackZoneType::BoostPad, ">> TEST NITRO PAD <<", sense::Color::Green());
        sense::track::addZone(curY + 900, curY + 1200, sense::TrackZoneType::OilSlick, "! TEST OIL SLICK !", sense::Color::Yellow());
        sense::track::addZone(curY + 1500, curY + 1800, sense::TrackZoneType::BoostPad, ">> TEST MEGA BOOST <<", sense::Color::Cyan());

        sense::render::showNotification("Spawned 3 Dynamic Track Zones Ahead!", 2.5f, sense::Color::Cyan());
    }
};

REGISTER_MOD(TestTrackModifiersMod)

