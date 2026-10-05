#pragma once

#include "types.hpp"
#include <string>
#include <cstdint>
#include <functional>

namespace sense {

// ============================================================================
// Steamworks API Controller & Integration
// ============================================================================
class SteamController {
public:
    static constexpr uint32_t DEFAULT_APP_ID = 3832650;
    static constexpr const char* DEFAULT_ACHIEVEMENT_ID = "HACK_ME";

    static SteamController& instance();

    // Lifecycle
    bool init(uint32_t appId = DEFAULT_APP_ID);
    void shutdown();
    void update(); // Calls SteamAPI_RunCallbacks()

    // Status Queries
    bool isInitialized() const { return m_initialized; }
    bool isSteamRunning() const;
    uint32_t getAppId() const { return m_appId; }
    std::string getPersonaName() const;
    uint64_t getSteamID() const;

    // Achievements & Stats for AppID (e.g. 3832650)
    bool unlockAchievement(const std::string& achievementId = DEFAULT_ACHIEVEMENT_ID);
    bool clearAchievement(const std::string& achievementId = DEFAULT_ACHIEVEMENT_ID);
    bool isAchievementUnlocked(const std::string& achievementId = DEFAULT_ACHIEVEMENT_ID);
    bool storeStats();

    // Overlay
    void activateOverlay(const std::string& dialog = "");

    // Game Completion Flow: Unlocks HACK_ME and shows in-game notification
    void triggerGameWinAchievement();
    bool hasAutoUnlockedHackMe() const { return m_autoUnlockedHackMe; }
    void resetWinTrigger() { m_autoUnlockedHackMe = false; }

private:
    SteamController();
    ~SteamController();

    bool loadSteamLibrary();
    void findInterfaces();

    HMODULE m_hSteamDll = nullptr;
    uint32_t m_appId = DEFAULT_APP_ID;
    bool m_initialized = false;
    bool m_autoUnlockedHackMe = false;

    // Native pointers
    void* m_pSteamUser = nullptr;
    void* m_pSteamUserStats = nullptr;
    void* m_pSteamFriends = nullptr;
};

// ============================================================================
// Convenience namespace
// ============================================================================
namespace steam {
    inline bool init(uint32_t appId = SteamController::DEFAULT_APP_ID) { return SteamController::instance().init(appId); }
    inline void shutdown() { SteamController::instance().shutdown(); }
    inline void update() { SteamController::instance().update(); }
    inline bool isInitialized() { return SteamController::instance().isInitialized(); }
    inline bool isSteamRunning() { return SteamController::instance().isSteamRunning(); }
    inline uint32_t getAppId() { return SteamController::instance().getAppId(); }
    inline std::string getPersonaName() { return SteamController::instance().getPersonaName(); }
    inline uint64_t getSteamID() { return SteamController::instance().getSteamID(); }
    inline bool unlockAchievement(const std::string& id = SteamController::DEFAULT_ACHIEVEMENT_ID) { return SteamController::instance().unlockAchievement(id); }
    inline bool clearAchievement(const std::string& id = SteamController::DEFAULT_ACHIEVEMENT_ID) { return SteamController::instance().clearAchievement(id); }
    inline bool isAchievementUnlocked(const std::string& id = SteamController::DEFAULT_ACHIEVEMENT_ID) { return SteamController::instance().isAchievementUnlocked(id); }
    inline bool storeStats() { return SteamController::instance().storeStats(); }
    inline void activateOverlay(const std::string& dlg = "") { SteamController::instance().activateOverlay(dlg); }
    inline void triggerGameWinAchievement() { SteamController::instance().triggerGameWinAchievement(); }
}

} // namespace sense

