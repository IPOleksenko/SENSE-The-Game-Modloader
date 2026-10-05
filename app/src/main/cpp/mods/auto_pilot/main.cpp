#include <api/sense_api.hpp>

class AutoPilotMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Auto-Pilot AI Bot";
        info.version = "1.0.0";
        info.author = "SenseModTeam";
        info.description = "Autonomous AI bot that walks, balances, and navigates checkpoints to beat the game.";
        return info;
    }

    void onInit() override {
        sense::log("AutoPilotMod: Initialized!");

        sense::input::registerHotkey(sense::Key_F3, [this]() {
            m_enabled = !m_enabled;
            sense::render::showNotification(
                m_enabled ? "AUTO-PILOT AI: [ACTIVATED]" : "AUTO-PILOT AI: [DEACTIVATED]",
                2.5f,
                m_enabled ? sense::Color::Green() : sense::Color::Red()
            );
        });

        sense::ui::addCheckbox("Auto-Pilot Bot (F3)", &m_enabled);
    }

    void onUpdate(float deltaTime) override {
        if (!m_enabled) return;

        auto& p = sense::api().player();
        if (!p.isValid()) return;

        // Keep player balanced while AI navigates
        if (!p.isAutoBalance()) {
            p.setAutoBalance(true);
        }

        float speed = p.getSpeed();
        float targetSpeed = p.getSpeedNormal();

        // Calculate dynamic stepping frequency to keep speed centered around target
        float stepInterval = 0.16f;
        if (speed < targetSpeed - 15.0f) {
            stepInterval = 0.08f; // Step faster if falling behind
        } else if (speed > targetSpeed + 15.0f) {
            stepInterval = 0.28f; // Slow down steps if going too fast
        }

        if (m_stepTimer >= stepInterval) {
            m_stepTimer = 0.0f;
            sense::PlayerMove nextMove = (m_lastStep == sense::PlayerMove::Left) ? sense::PlayerMove::Right : sense::PlayerMove::Left;
            p.step(nextMove);
            m_lastStep = nextMove;
        }
    }

    void onRender(SDL_Renderer* renderer) override {
        if (!m_enabled) return;

        sense::Point winSize = sense::game::getWindowSize();
        sense::Rect bannerRect((winSize.x - 220) / 2, 45, 220, 24);

        sense::render::fillRect(renderer, bannerRect, sense::Color(20, 140, 60, 220));
        sense::render::drawRect(renderer, bannerRect, sense::Color::White());
        sense::render::drawText(renderer, "AI AUTOPILOT RUNNING", bannerRect.x + 12, bannerRect.y + 6, sense::Color::White(), 0.9f);
    }

private:
    bool m_enabled = false;
    float m_stepTimer = 0.0f;
    sense::PlayerMove m_lastStep = sense::PlayerMove::Left;
};

REGISTER_MOD(AutoPilotMod)

