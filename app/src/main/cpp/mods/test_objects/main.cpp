#include <api/sense_api.hpp>

class TestObjectsMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Test: 2D & 3D Objects Subsystem";
        info.version = "1.0.0";
        info.author = "SenseTestSuite";
        info.description = "Test harness for procedural 3D world objects, spinning coins, pyramids, crystals, arches, and custom 2D entities.";
        return info;
    }

    void onInit() override {
        sense::log("TestObjectsMod: Initializing 2D & 3D objects test harness!");

        // Spawn welcoming arch at start of track
        sense::objects::spawnArch(100.0f, 380.0f, 220.0f, sense::Color::Green());

        // Spawn rotating coins along early track
        for (int y = 300; y <= 1500; y += 200) {
            sense::objects::spawnCoin({ -60.0f, static_cast<float>(y), 25.0f }, 22.0f, sense::Color(255, 215, 0));
            sense::objects::spawnCoin({ 60.0f, static_cast<float>(y), 25.0f }, 22.0f, sense::Color(255, 215, 0));
        }

        // Spawn floating neon pyramids and crystals
        sense::objects::spawnPyramid({ -180.0f, 800.0f, 30.0f }, 50.0f, 70.0f, sense::Color::Yellow());
        sense::objects::spawnCrystal({ 180.0f, 1200.0f, 40.0f }, 35.0f, 80.0f, sense::Color::Magenta());

        // Spawn 2D billboard entities
        sense::objects::spawnEntity2D(-120.0f, 500.0f, 80.0f, 40.0f, sense::Color::Cyan(), nullptr, "SPEED ZONE");

        // F1 Menu UI Controls
        sense::ui::addSection("API Test: 2D & 3D World Objects");
        sense::ui::addButton("Spawn 3D Coin Ahead", [this]() {
            float playerY = static_cast<float>(sense::player::getY());
            sense::objects::spawnCoin({ 0.0f, playerY + 300.0f, 30.0f }, 25.0f, sense::Color(255, 215, 0));
            sense::render::showNotification("Spawned 3D Gold Coin Ahead!", 2.0f, sense::Color(255, 215, 0));
        });

        sense::ui::addButton("Spawn 3D Crystal Ahead", [this]() {
            float playerY = static_cast<float>(sense::player::getY());
            sense::objects::spawnCrystal({ -50.0f, playerY + 400.0f, 40.0f }, 35.0f, 75.0f, sense::Color::Magenta());
            sense::render::showNotification("Spawned 3D Neon Crystal Ahead!", 2.0f, sense::Color::Magenta());
        });

        sense::ui::addButton("Spawn 3D Checkpoint Arch Ahead", [this]() {
            float playerY = static_cast<float>(sense::player::getY());
            sense::objects::spawnArch(playerY + 500.0f, 380.0f, 220.0f, sense::Color::Cyan());
            sense::render::showNotification("Spawned 3D Finish Arch Ahead!", 2.0f, sense::Color::Cyan());
        });

        sense::ui::addButton("Clear All Custom Objects", []() {
            sense::objects::clearAll();
            sense::render::showNotification("All Custom Objects Cleared", 2.0f, sense::Color::Red());
        });
    }
};

REGISTER_MOD(TestObjectsMod)

