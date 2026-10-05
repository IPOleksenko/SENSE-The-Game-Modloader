#include "achievements.hpp"
#include "render.hpp"
#include "audio.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace sense {

AchievementController& AchievementController::instance() {
    static AchievementController inst;
    return inst;
}

AchievementController::AchievementController() {
    // Default built-in challenges for SENSE: The Game
    Achievement a1;
    a1.id = "first_steps";
    a1.title = "First Steps";
    a1.description = "Begin your journey and travel 500 meters.";
    a1.points = 10;
    registerAchievement(a1);

    Achievement a2;
    a2.id = "speed_demon";
    a2.title = "Speed Demon";
    a2.description = "Accelerate beyond 135 KM/H!";
    a2.points = 25;
    registerAchievement(a2);

    Achievement a3;
    a3.id = "tightrope_master";
    a3.title = "Master of Equilibrium";
    a3.description = "Reach Sector 2 without wiping out.";
    a3.points = 50;
    registerAchievement(a3);
}

void AchievementController::registerAchievement(const Achievement& achievement) {
    for (auto& a : m_achievements) {
        if (a.id == achievement.id) {
            a = achievement;
            return;
        }
    }
    m_achievements.push_back(achievement);
}

Achievement* AchievementController::findAchievement(const std::string& id) {
    for (auto& a : m_achievements) {
        if (a.id == id) return &a;
    }
    return nullptr;
}

void AchievementController::unlock(const std::string& id) {
    auto* ach = findAchievement(id);
    if (!ach || ach->unlocked) return;

    ach->unlocked = true;
    ach->progress = 100.0f;

    AchievementNotification notif;
    notif.achievement = *ach;
    notif.timer = 0.0f;
    notif.duration = 4.0f;
    notif.slideY = -70.0f;
    m_notifications.push_back(notif);

    if (m_onUnlock) {
        m_onUnlock(*ach);
    }
}

void AchievementController::setProgress(const std::string& id, float progress) {
    auto* ach = findAchievement(id);
    if (!ach) return;

    ach->progress = (std::max)(0.0f, (std::min)(100.0f, progress));
    if (ach->progress >= 100.0f && !ach->unlocked) {
        unlock(id);
    }
}

float AchievementController::getProgress(const std::string& id) const {
    for (const auto& a : m_achievements) {
        if (a.id == id) return a.progress;
    }
    return 0.0f;
}

bool AchievementController::isUnlocked(const std::string& id) const {
    for (const auto& a : m_achievements) {
        if (a.id == id) return a.unlocked;
    }
    return false;
}

void AchievementController::saveToFile(const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return;
    for (const auto& a : m_achievements) {
        out << a.id << "=" << (a.unlocked ? "1" : "0") << "," << a.progress << "\n";
    }
}

void AchievementController::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        std::string id = line.substr(0, eqPos);
        std::string rest = line.substr(eqPos + 1);
        auto commaPos = rest.find(',');
        bool unlocked = (rest.substr(0, commaPos) == "1");
        float prog = (commaPos != std::string::npos) ? std::stof(rest.substr(commaPos + 1)) : (unlocked ? 100.0f : 0.0f);

        auto* a = findAchievement(id);
        if (a) {
            a->unlocked = unlocked;
            a->progress = prog;
        }
    }
}

void AchievementController::update(float deltaTime) {
    if (m_notifications.empty()) return;

    auto& active = m_notifications.front();
    active.timer += deltaTime;

    // Smooth ease-in / ease-out slide animation
    float targetY = 24.0f;
    if (active.timer < 0.4f) {
        float t = active.timer / 0.4f;
        active.slideY = -70.0f + (targetY - -70.0f) * std::sin(t * 1.57079f);
    } else if (active.timer > active.duration - 0.4f) {
        float t = (active.duration - active.timer) / 0.4f;
        active.slideY = -70.0f + (targetY - -70.0f) * std::sin(t * 1.57079f);
    } else {
        active.slideY = targetY;
    }

    if (active.timer >= active.duration) {
        m_notifications.pop_front();
    }
}

void AchievementController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (m_notifications.empty() || !renderer) return;

    const auto& active = m_notifications.front();
    auto& rc = RenderController::instance();

    int panelW = 340;
    int panelH = 58;
    int panelX = (screenW - panelW) / 2;
    int panelY = static_cast<int>(active.slideY);

    if (panelY <= -58) return;

    // Shadow & Background
    rc.fillRect(renderer, Rect(panelX + 4, panelY + 4, panelW, panelH), Color(0, 0, 0, 140));
    rc.fillRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(16, 20, 28, 245));
    rc.drawRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(255, 200, 40, 230));

    // Golden Trophy Emblem Box
    rc.fillRect(renderer, Rect(panelX + 8, panelY + 8, 42, 42), Color(255, 180, 0, 220));
    rc.drawRect(renderer, Rect(panelX + 8, panelY + 8, 42, 42), Color::White());
    rc.drawText(renderer, "[*]", panelX + 18, panelY + 20, Color::White(), 1.0f);

    // Title & Category
    rc.drawText(renderer, "ACHIEVEMENT UNLOCKED!", panelX + 58, panelY + 8, Color(255, 215, 0), 0.85f);
    rc.drawText(renderer, active.achievement.title, panelX + 58, panelY + 24, Color::White(), 1.0f);
    rc.drawText(renderer, active.achievement.description, panelX + 58, panelY + 40, Color(180, 190, 210), 0.75f);

    // Points badge on right
    char pts[16];
    std::snprintf(pts, sizeof(pts), "+%d PTS", active.achievement.points);
    rc.drawText(renderer, pts, panelX + panelW - 65, panelY + 8, Color(100, 255, 120), 0.85f);
}

} // namespace sense

