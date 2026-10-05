#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <deque>
#include <functional>

namespace sense {

struct Achievement {
    std::string id;
    std::string title;
    std::string description;
    int points = 10;
    float progress = 0.0f; // 0.0f to 100.0f
    bool unlocked = false;
    bool hidden = false;
};

struct AchievementNotification {
    Achievement achievement;
    float timer = 0.0f;
    float duration = 4.0f;
    float slideY = 0.0f;
};

class AchievementController {
public:
    static AchievementController& instance();

    void registerAchievement(const Achievement& achievement);
    void unlock(const std::string& id);
    void setProgress(const std::string& id, float progress);
    float getProgress(const std::string& id) const;
    bool isUnlocked(const std::string& id) const;

    const std::vector<Achievement>& getAll() const { return m_achievements; }
    Achievement* findAchievement(const std::string& id);

    void saveToFile(const std::string& filepath);
    void loadFromFile(const std::string& filepath);

    void update(float deltaTime);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

    void setOnUnlockCallback(std::function<void(const Achievement&)> cb) { m_onUnlock = cb; }

private:
    AchievementController();

    std::vector<Achievement> m_achievements;
    std::deque<AchievementNotification> m_notifications;
    std::function<void(const Achievement&)> m_onUnlock;
};

namespace achievements {
    inline void registerAchievement(const Achievement& ach) { AchievementController::instance().registerAchievement(ach); }
    inline void unlock(const std::string& id) { AchievementController::instance().unlock(id); }
    inline void setProgress(const std::string& id, float p) { AchievementController::instance().setProgress(id, p); }
    inline bool isUnlocked(const std::string& id) { return AchievementController::instance().isUnlocked(id); }
    inline float getProgress(const std::string& id) { return AchievementController::instance().getProgress(id); }
}

} // namespace sense

