#include <api/sense_api.hpp>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

class SpeedometerMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Digital Speedometer & HUD";
        info.version = "1.0.0";
        info.author = "SenseModTeam";
        info.description = "Telemetry HUD displaying real-time speed in KM/H, balance meter, and route progress.";
        return info;
    }

    void onInit() override {
        sense::log("SpeedometerMod: Initializing telemetry HUD!");

        // Toggle HUD via hotkey F2
        sense::input::registerHotkey(sense::Key_F2, [this]() {
            m_visible = !m_visible;
            sense::render::showNotification(
                m_visible ? "SPEEDOMETER HUD: [VISIBLE]" : "SPEEDOMETER HUD: [HIDDEN]",
                2.0f,
                m_visible ? sense::Color::Green() : sense::Color::Yellow()
            );
        });

        // Add section and toggle to mod menu
        sense::ui::addSection("Telemetry & Navigation");
        sense::ui::addCheckbox("Show Speedometer HUD (F2)", &m_visible);
    }

    void onUpdate(float deltaTime) override {
        if (!m_visible) return;

        auto& player = sense::api().player();
        if (!player.isValid()) return;

        float currentSpeed = player.getSpeed();
        if (currentSpeed > m_maxSpeedAchieved) {
            m_maxSpeedAchieved = currentSpeed;
        }

        // Smooth speed interpolation for HUD display
        m_smoothedSpeed += (currentSpeed - m_smoothedSpeed) * std::min(1.0f, deltaTime * 12.0f);
    }

    void onRender(SDL_Renderer* renderer) override {
        if (!m_visible) return;

        auto& player = sense::api().player();
        if (!player.isValid()) return;

        auto& rc = sense::api().render();
        sense::Point winSize = sense::game::getWindowSize();

        // Position HUD in top-right corner
        const int hudW = 280;
        const int hudH = 150;
        const int hudX = winSize.x - hudW - 20;
        const int hudY = 20;

        sense::Rect hudRect(hudX, hudY, hudW, hudH);
        sense::Rect shadowRect(hudX + 4, hudY + 4, hudW, hudH);

        // Shadow & Background
        rc.fillRect(renderer, shadowRect, sense::Color(0, 0, 0, 120));
        rc.fillRect(renderer, hudRect, sense::Color(14, 18, 28, 230));
        rc.drawRect(renderer, hudRect, sense::Color(45, 120, 220, 220));

        // Header Title
        rc.fillRect(renderer, sense::Rect(hudX, hudY, hudW, 22), sense::Color(25, 35, 55, 255));
        rc.drawText(renderer, "TELEMETRY & SPEED HUD [F2]", hudX + 10, hudY + 5, sense::Color::Cyan(), 0.85f);

        // 1. Digital Speed (converted to virtual KM/H: speed * 0.72)
        float kmh = m_smoothedSpeed * 0.72f;
        std::ostringstream ssKmh;
        ssKmh << std::fixed << std::setprecision(1) << kmh << " KM/H";

        sense::Color speedColor = sense::Color::Green();
        if (m_smoothedSpeed > player.getSpeedNormal() + 20.0f) {
            speedColor = sense::Color::Red();
        } else if (m_smoothedSpeed > player.getSpeedNormal()) {
            speedColor = sense::Color::Yellow();
        }

        rc.drawText(renderer, ssKmh.str(), hudX + 12, hudY + 28, speedColor, 1.4f);

        // Max Speed Stat
        std::ostringstream ssMax;
        ssMax << "MAX: " << std::fixed << std::setprecision(1) << (m_maxSpeedAchieved * 0.72f) << " KM/H";
        rc.drawText(renderer, ssMax.str(), hudX + 160, hudY + 32, sense::Color::Gray(180), 0.85f);

        // 2. Speed Bar
        int barX = hudX + 12;
        int barY = hudY + 54;
        int barW = hudW - 24;
        int barH = 12;

        float speedFrac = (m_smoothedSpeed - player.getSpeedMin()) / (player.getSpeedMax() - player.getSpeedMin());
        speedFrac = std::clamp(speedFrac, 0.0f, 1.0f);

        rc.fillRect(renderer, sense::Rect(barX, barY, barW, barH), sense::Color(30, 40, 55, 255));
        rc.fillRect(renderer, sense::Rect(barX, barY, static_cast<int>(barW * speedFrac), barH), speedColor);
        rc.drawRect(renderer, sense::Rect(barX, barY, barW, barH), sense::Color::Gray(140));

        // Target Speed Marker line
        float normFrac = (player.getSpeedNormal() - player.getSpeedMin()) / (player.getSpeedMax() - player.getSpeedMin());
        int normX = barX + static_cast<int>(barW * normFrac);
        rc.drawLine(renderer, normX, barY - 2, normX, barY + barH + 2, sense::Color::White());

        // 3. Balance Indicator
        int balY = hudY + 76;
        float balance = player.getBalance();
        bool isBalanced = player.isBalanced();

        rc.drawText(renderer, "BALANCE:", hudX + 12, balY, sense::Color::Yellow(), 0.85f);

        std::string balStatus = isBalanced ? "STABLE" : (balance < 0.0f ? "LEANING LEFT" : "LEANING RIGHT");
        sense::Color balColor = isBalanced ? sense::Color::Green() : sense::Color(255, 120, 40);
        rc.drawText(renderer, balStatus, hudX + 150, balY, balColor, 0.85f);

        int balBarX = hudX + 12;
        int balBarY = balY + 16;
        int balBarW = hudW - 24;
        int balBarH = 8;
        int balCenter = balBarX + (balBarW / 2);

        rc.fillRect(renderer, sense::Rect(balBarX, balBarY, balBarW, balBarH), sense::Color(30, 40, 55, 255));
        rc.drawLine(renderer, balCenter, balBarY - 2, balCenter, balBarY + balBarH + 2, sense::Color::White());

        // Draw balance offset bar from center
        float balOffset = std::clamp(balance * 2.0f, -1.0f, 1.0f);
        int fillW = static_cast<int>(balOffset * (balBarW / 2));
        if (fillW > 0) {
            rc.fillRect(renderer, sense::Rect(balCenter, balBarY, fillW, balBarH), balColor);
        } else if (fillW < 0) {
            rc.fillRect(renderer, sense::Rect(balCenter + fillW, balBarY, -fillW, balBarH), balColor);
        }
        rc.drawRect(renderer, sense::Rect(balBarX, balBarY, balBarW, balBarH), sense::Color::Gray(140));

        // 4. Track Distance & Progress Bar
        int progY = hudY + 110;
        int curY = player.getY();
        const int finishY = static_cast<int>(sense::CheckPoint::FINAL_START); // 25000 px
        float progressFrac = std::clamp(static_cast<float>(curY) / finishY, 0.0f, 1.0f);

        std::ostringstream ssProg;
        ssProg << "PROGRESS: " << std::fixed << std::setprecision(1) << (progressFrac * 100.0f) << "% (" << curY << " px)";
        rc.drawText(renderer, ssProg.str(), hudX + 12, progY, sense::Color::Cyan(), 0.85f);

        int prgBarX = hudX + 12;
        int prgBarY = progY + 16;
        int prgBarW = hudW - 24;
        int prgBarH = 8;

        rc.fillRect(renderer, sense::Rect(prgBarX, prgBarY, prgBarW, prgBarH), sense::Color(30, 40, 55, 255));
        rc.fillRect(renderer, sense::Rect(prgBarX, prgBarY, static_cast<int>(prgBarW * progressFrac), prgBarH), sense::Color::Cyan());
        rc.drawRect(renderer, sense::Rect(prgBarX, prgBarY, prgBarW, prgBarH), sense::Color::Gray(140));
    }

private:
    bool m_visible = true;
    float m_smoothedSpeed = 0.0f;
    float m_maxSpeedAchieved = 0.0f;
};

REGISTER_MOD(SpeedometerMod)
