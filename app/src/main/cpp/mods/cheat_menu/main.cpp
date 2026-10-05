#include <api/sense_api.hpp>

class CheatMenuMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Trainer & Cheats";
        info.version = "1.0.0";
        info.author = "SenseModTeam";
        info.description = "Full trainer mod with God Mode, Auto-Balance, Teleports, and Speedhack.";
        return info;
    }

    void onInit() override {
        sense::log("CheatMenuMod: Initializing trainer hotkeys!");

        // F4: God Mode
        sense::input::registerHotkey(sense::Key_F4, []() {
            auto& player = sense::api().player();
            player.setGodMode(!player.isGodMode());
            sense::render::showNotification(
                player.isGodMode() ? "God Mode: [ENABLED]" : "God Mode: [DISABLED]",
                2.0f,
                player.isGodMode() ? sense::Color::Green() : sense::Color::Red()
            );
        });

        // F5: Auto Balancer
        sense::input::registerHotkey(sense::Key_F5, []() {
            auto& player = sense::api().player();
            player.setAutoBalance(!player.isAutoBalance());
            sense::render::showNotification(
                player.isAutoBalance() ? "Auto-Balancer: [ACTIVE]" : "Auto-Balancer: [OFF]",
                2.0f,
                player.isAutoBalance() ? sense::Color::Green() : sense::Color::Red()
            );
        });

        // F6: Speedhack (2.0x)
        sense::input::registerHotkey(sense::Key_F6, [this]() {
            m_speedhackActive = !m_speedhackActive;
            sense::game::setTimeScale(m_speedhackActive ? 2.0f : 1.0f);
            sense::render::showNotification(
                m_speedhackActive ? "Speedhack: 2.0x [ACTIVE]" : "Speedhack: Normal [1.0x]",
                2.0f,
                m_speedhackActive ? sense::Color::Magenta() : sense::Color::White()
            );
        });

        // F7: Warp +1000 px
        sense::input::registerHotkey(sense::Key_F7, []() {
            auto& player = sense::api().player();
            if (player.isValid()) {
                player.teleport(player.getY() + 1000);
                sense::render::showNotification("Warped +1000 px!", 1.5f, sense::Color::Cyan());
            }
        });

        // F12: Teleport to Final Checkpoint
        sense::input::registerHotkey(sense::Key_F12, []() {
            auto& player = sense::api().player();
            if (player.isValid()) {
                player.teleportToCheckpoint(sense::CheckPoint::FINAL_START);
                sense::render::showNotification("Teleported to Final Checkpoint!", 2.0f, sense::Color::Yellow());
            }
        });

        // Add to mod menu
        sense::ui::addSection("Trainer Controls");
        sense::ui::addCheckbox("God Mode (F4)", &m_godModeProxy);
        sense::ui::addCheckbox("Auto-Balance (F5)", &m_autoBalanceProxy);
        sense::ui::addButton("Warp Forward (F7)", []() {
            sense::player::teleport(sense::player::getY() + 1000);
        });
        sense::ui::addButton("Toggle Endless Mode (Space)", []() {
            sense::game::toggleEndlessMode();
        });
    }

    void onUpdate(float deltaTime) override {
        // Sync proxies with player state
        auto& p = sense::api().player();
        if (p.isValid()) {
            if (m_godModeProxy != p.isGodMode()) p.setGodMode(m_godModeProxy);
            if (m_autoBalanceProxy != p.isAutoBalance()) p.setAutoBalance(m_autoBalanceProxy);
        }
    }

    void onRender(SDL_Renderer* renderer) override {
        // Render trainer badges in top-left
        auto& p = sense::api().player();
        int badgeY = 12;

        if (p.isGodMode()) {
            sense::render::fillRect(renderer, sense::Rect(10, badgeY, 130, 20), sense::Color(40, 160, 60, 220));
            sense::render::drawText(renderer, "GOD MODE: ON", 18, badgeY + 4, sense::Color::White(), 0.9f);
            badgeY += 24;
        }

        if (p.isAutoBalance()) {
            sense::render::fillRect(renderer, sense::Rect(10, badgeY, 150, 20), sense::Color(30, 120, 200, 220));
            sense::render::drawText(renderer, "AUTO-BALANCE: ON", 18, badgeY + 4, sense::Color::White(), 0.9f);
            badgeY += 24;
        }

        if (m_speedhackActive) {
            sense::render::fillRect(renderer, sense::Rect(10, badgeY, 140, 20), sense::Color(180, 50, 180, 220));
            sense::render::drawText(renderer, "SPEEDHACK: 2.0x", 18, badgeY + 4, sense::Color::White(), 0.9f);
            badgeY += 24;
        }
    }

private:
    bool m_speedhackActive = false;
    bool m_godModeProxy = false;
    bool m_autoBalanceProxy = false;
};

REGISTER_MOD(CheatMenuMod)

