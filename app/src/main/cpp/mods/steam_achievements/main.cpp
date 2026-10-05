#include <api/sense_api.hpp>
#include <api/steam.hpp>

// steam_achievements — production mod.
// Grants the "HACK_ME" Steam achievement (AppID 3832650) when the player wins the game.
class SteamAchievementsMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name    = "Steam Achievements";
        info.version = "1.0.0";
        info.author  = "ModLoader";
        info.description = "Grants the 'HACK_ME' Steam achievement (AppID 3832650) upon game completion.";
        return info;
    }

    void onPlayerWin() override {
        auto& steam = sense::SteamController::instance();
        if (!steam.isInitialized()) {
            sense::log("SteamAchievements: Steam not initialized, skipping achievement.");
            return;
        }
        if (steam.isAchievementUnlocked("HACK_ME")) {
            sense::log("SteamAchievements: 'HACK_ME' already unlocked.");
            return;
        }
        bool ok = steam.unlockAchievement("HACK_ME");
        sense::log("SteamAchievements: unlockAchievement('HACK_ME') -> %s", ok ? "OK" : "FAILED");
        sense::render::showNotification(
            ok ? "[Steam] Achievement 'HACK_ME' unlocked!" : "[Steam] Achievement unlock failed (offline?)",
            5.0f,
            ok ? sense::Color(255, 215, 0) : sense::Color::Red()
        );
    }
};

REGISTER_MOD(SteamAchievementsMod)

