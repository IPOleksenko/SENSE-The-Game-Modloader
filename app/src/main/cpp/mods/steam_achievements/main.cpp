#include <api/sense_api.hpp>
#include <api/steam.hpp>

// steam_achievements — production mod.
// Grants the "THE UNIVERSE DOESN'T MAKE SENSE." Steam achievement (AppID 3832650) when the player wins the game.
class SteamAchievementsMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name    = "Steam Achievements";
        info.version = "1.0.0";
        info.author  = "ModLoader";
        info.description = "Grants the 'SENSE: The Game' Steam achievement (AppID 3832650) upon game completion.";
        return info;
    }

    void onPlayerWin() override {
        auto& steam = sense::SteamController::instance();
        if (!steam.isInitialized()) {
            sense::log("SteamAchievements: Steam not initialized, skipping achievement.");
            return;
        }
        if (steam.isAchievementUnlocked("SENSE: The Game")) {
            sense::log("SteamAchievements: 'THE UNIVERSE DOESN'T MAKE SENSE.' already unlocked.");
            return;
        }
        bool ok = steam.unlockAchievement("SENSE: The Game");
        sense::log("SteamAchievements: unlockAchievement('SENSE: The Game') -> %s", ok ? "OK" : "FAILED");
        sense::render::showNotification(
            ok ? "[Steam] Achievement 'THE UNIVERSE DOESN'T MAKE SENSE.' unlocked!" : "[Steam] Achievement unlock failed (offline?)",
            5.0f,
            ok ? sense::Color(255, 215, 0) : sense::Color::Red()
        );
    }
};

REGISTER_MOD(SteamAchievementsMod)

