#include "steam.hpp"
#include "sense_api.hpp"
#include <fstream>
#include <iostream>

namespace sense {

// Steamworks Flat Function Pointers
typedef int (*PFN_SteamAPI_InitFlat)(char* pszErrMsg);
typedef bool (*PFN_SteamAPI_Init)();
typedef void (*PFN_SteamAPI_Shutdown)();
typedef void (*PFN_SteamAPI_RunCallbacks)();
typedef bool (*PFN_SteamAPI_IsSteamRunning)();

typedef void* (*PFN_GetInterface)();

typedef bool (*PFN_SetAchievement)(void* pStats, const char* pchName);
typedef bool (*PFN_ClearAchievement)(void* pStats, const char* pchName);
typedef bool (*PFN_GetAchievement)(void* pStats, const char* pchName, bool* pbAchieved);
typedef bool (*PFN_StoreStats)(void* pStats);

typedef const char* (*PFN_GetPersonaName)(void* pFriends);
typedef void (*PFN_ActivateGameOverlay)(void* pFriends, const char* pchDialog);
typedef uint64_t (*PFN_GetSteamID)(void* pUser);

static PFN_SteamAPI_InitFlat        g_fnInitFlat = nullptr;
static PFN_SteamAPI_Init            g_fnInit = nullptr;
static PFN_SteamAPI_Shutdown        g_fnShutdown = nullptr;
static PFN_SteamAPI_RunCallbacks    g_fnRunCallbacks = nullptr;
static PFN_SteamAPI_IsSteamRunning  g_fnIsSteamRunning = nullptr;

static PFN_SetAchievement           g_fnSetAchievement = nullptr;
static PFN_ClearAchievement         g_fnClearAchievement = nullptr;
static PFN_GetAchievement           g_fnGetAchievement = nullptr;
static PFN_StoreStats               g_fnStoreStats = nullptr;

static PFN_GetPersonaName           g_fnGetPersonaName = nullptr;
static PFN_ActivateGameOverlay      g_fnActivateGameOverlay = nullptr;
static PFN_GetSteamID               g_fnGetSteamID = nullptr;

// Safe helper to find an exported versioned interface via GetProcAddress (e.g. SteamAPI_SteamUserStats_v013 down to v001)
static FARPROC findSteamInterfaceExport(HMODULE hMod, const char* prefix, int maxVer = 30) {
    if (!hMod || !prefix) return nullptr;

    char sym[128] = {};
    for (int v = maxVer; v >= 1; --v) {
        snprintf(sym, sizeof(sym), "%s%03d", prefix, v);
        FARPROC proc = GetProcAddress(hMod, sym);
        if (proc) return proc;
    }
    // Also try without version suffix
    return GetProcAddress(hMod, prefix);
}

SteamController& SteamController::instance() {
    static SteamController s_instance;
    return s_instance;
}

SteamController::SteamController() = default;

SteamController::~SteamController() {
    shutdown();
}

bool SteamController::loadSteamLibrary() {
    if (m_hSteamDll) return true;

    // Check if already in process
    m_hSteamDll = GetModuleHandleA("steam_api64.dll");
    if (m_hSteamDll) return true;

    // 1. Current working directory
    m_hSteamDll = LoadLibraryA("steam_api64.dll");
    if (m_hSteamDll) return true;

    // 2. Next to executable
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH)) {
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            std::string dllInExeDir = std::string(exePath) + "steam_api64.dll";
            m_hSteamDll = LoadLibraryA(dllInExeDir.c_str());
            if (m_hSteamDll) return true;
        }
    }

    // 3. Check assets or common mod directories
    m_hSteamDll = LoadLibraryA("assets\\steam_api64.dll");
    if (m_hSteamDll) return true;

    m_hSteamDll = LoadLibraryA("..\\assets\\steam_api64.dll");
    if (m_hSteamDll) return true;

    // 4. Check Steam installation registry path
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char steamPath[MAX_PATH] = {};
        DWORD pathSize = sizeof(steamPath);
        if (RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamPath), &pathSize) == ERROR_SUCCESS) {
            std::string steamDll = std::string(steamPath) + "\\steam_api64.dll";
            m_hSteamDll = LoadLibraryA(steamDll.c_str());
        }
        RegCloseKey(hKey);
        if (m_hSteamDll) return true;
    }

    return m_hSteamDll != nullptr;
}

void SteamController::findInterfaces() {
    if (!m_hSteamDll) return;

    // Resolve general Steam API entrypoints
    g_fnInitFlat       = reinterpret_cast<PFN_SteamAPI_InitFlat>(GetProcAddress(m_hSteamDll, "SteamAPI_InitFlat"));
    g_fnInit           = reinterpret_cast<PFN_SteamAPI_Init>(GetProcAddress(m_hSteamDll, "SteamAPI_InitSafe"));
    if (!g_fnInit) {
        g_fnInit       = reinterpret_cast<PFN_SteamAPI_Init>(GetProcAddress(m_hSteamDll, "SteamAPI_Init"));
    }
    g_fnShutdown       = reinterpret_cast<PFN_SteamAPI_Shutdown>(GetProcAddress(m_hSteamDll, "SteamAPI_Shutdown"));
    g_fnRunCallbacks   = reinterpret_cast<PFN_SteamAPI_RunCallbacks>(GetProcAddress(m_hSteamDll, "SteamAPI_RunCallbacks"));
    g_fnIsSteamRunning = reinterpret_cast<PFN_SteamAPI_IsSteamRunning>(GetProcAddress(m_hSteamDll, "SteamAPI_IsSteamRunning"));

    // User Stats functions
    g_fnSetAchievement   = reinterpret_cast<PFN_SetAchievement>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamUserStats_SetAchievement"));
    g_fnClearAchievement = reinterpret_cast<PFN_ClearAchievement>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamUserStats_ClearAchievement"));
    g_fnGetAchievement   = reinterpret_cast<PFN_GetAchievement>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamUserStats_GetAchievement"));
    g_fnStoreStats       = reinterpret_cast<PFN_StoreStats>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamUserStats_StoreStats"));

    // Friends & Overlay
    g_fnGetPersonaName      = reinterpret_cast<PFN_GetPersonaName>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamFriends_GetPersonaName"));
    g_fnActivateGameOverlay = reinterpret_cast<PFN_ActivateGameOverlay>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamFriends_ActivateGameOverlay"));

    // User
    g_fnGetSteamID          = reinterpret_cast<PFN_GetSteamID>(GetProcAddress(m_hSteamDll, "SteamAPI_ISteamUser_GetSteamID"));
}

bool SteamController::init(uint32_t appId) {
    if (m_initialized) return true;

    m_appId = appId;

    // Set Steam AppId environment variables
    std::string appIdStr = std::to_string(m_appId);
    SetEnvironmentVariableA("SteamAppId", appIdStr.c_str());
    SetEnvironmentVariableA("SteamGameId", appIdStr.c_str());

    // Write steam_appid.txt if missing
    std::ifstream checkFile("steam_appid.txt");
    if (!checkFile.is_open()) {
        std::ofstream appFile("steam_appid.txt");
        if (appFile.is_open()) {
            appFile << appIdStr << "\n";
            appFile.close();
        }
    } else {
        checkFile.close();
    }

    if (!loadSteamLibrary()) {
        log("Steam: steam_api64.dll could not be located on system.");
        return false;
    }

    findInterfaces();

    // Try initializing via Flat API first, fallback to standard Init
    bool initOk = false;
    if (g_fnInitFlat) {
        char errBuf[1024] = {};
        int result = g_fnInitFlat(errBuf);
        if (result == 0) { // 0 = k_ESteamAPIInitResult_OK
            initOk = true;
        } else {
            log("Steam: SteamAPI_InitFlat returned non-zero (%d): %s", result, errBuf);
        }
    } else if (g_fnInit) {
        initOk = g_fnInit();
    }

    if (!initOk) {
        log("Steam: Initialization failed (Steam client may be closed or offline).");
        return false;
    }

    // Query active interfaces only AFTER SteamAPI initialized successfully
    auto fnGetUserStats = reinterpret_cast<PFN_GetInterface>(findSteamInterfaceExport(m_hSteamDll, "SteamAPI_SteamUserStats_v", 30));
    if (fnGetUserStats) {
        m_pSteamUserStats = fnGetUserStats();
    }

    auto fnGetFriends = reinterpret_cast<PFN_GetInterface>(findSteamInterfaceExport(m_hSteamDll, "SteamAPI_SteamFriends_v", 30));
    if (fnGetFriends) {
        m_pSteamFriends = fnGetFriends();
    }

    auto fnGetUser = reinterpret_cast<PFN_GetInterface>(findSteamInterfaceExport(m_hSteamDll, "SteamAPI_SteamUser_v", 30));
    if (fnGetUser) {
        m_pSteamUser = fnGetUser();
    }

    m_initialized = true;
    log("Steam: Initialized successfully for AppID %u! User: %s (SteamID: %llu)",
        m_appId, getPersonaName().c_str(), getSteamID());

    return true;
}

void SteamController::shutdown() {
    if (!m_initialized) return;

    if (g_fnShutdown) {
        g_fnShutdown();
    }

    m_initialized = false;
    m_pSteamUser = nullptr;
    m_pSteamUserStats = nullptr;
    m_pSteamFriends = nullptr;
    log("Steam: Shutdown complete.");
}

void SteamController::update() {
    if (!m_initialized) return;

    if (g_fnRunCallbacks) {
        g_fnRunCallbacks();
    }
}

bool SteamController::isSteamRunning() const {
    if (g_fnIsSteamRunning) {
        return g_fnIsSteamRunning();
    }
    return m_initialized;
}

std::string SteamController::getPersonaName() const {
    if (m_initialized && m_pSteamFriends && g_fnGetPersonaName) {
        const char* name = g_fnGetPersonaName(m_pSteamFriends);
        if (name) return std::string(name);
    }
    return "Unknown";
}

uint64_t SteamController::getSteamID() const {
    if (m_initialized && m_pSteamUser && g_fnGetSteamID) {
        return g_fnGetSteamID(m_pSteamUser);
    }
    return 0;
}

bool SteamController::unlockAchievement(const std::string& achievementId) {
    if (!m_initialized) {
        if (!init(m_appId)) return false;
    }

    if (!m_pSteamUserStats || !g_fnSetAchievement) {
        log("Steam: UserStats interface unavailable for SetAchievement.");
        return false;
    }

    bool success = g_fnSetAchievement(m_pSteamUserStats, achievementId.c_str());
    if (success) {
        storeStats();
        log("Steam: SetAchievement('%s') succeeded for AppID %u.", achievementId.c_str(), m_appId);
    } else {
        log("Steam: SetAchievement('%s') failed for AppID %u.", achievementId.c_str(), m_appId);
    }
    return success;
}

bool SteamController::clearAchievement(const std::string& achievementId) {
    if (!m_initialized) {
        if (!init(m_appId)) return false;
    }

    if (!m_pSteamUserStats || !g_fnClearAchievement) {
        log("Steam: UserStats interface unavailable for ClearAchievement.");
        return false;
    }

    bool success = g_fnClearAchievement(m_pSteamUserStats, achievementId.c_str());
    if (success) {
        storeStats();
        log("Steam: ClearAchievement('%s') succeeded for AppID %u.", achievementId.c_str(), m_appId);
    } else {
        log("Steam: ClearAchievement('%s') failed for AppID %u.", achievementId.c_str(), m_appId);
    }
    return success;
}

bool SteamController::isAchievementUnlocked(const std::string& achievementId) {
    if (!m_initialized) {
        if (!init(m_appId)) return false;
    }

    if (!m_pSteamUserStats || !g_fnGetAchievement) {
        return false;
    }

    bool achieved = false;
    if (g_fnGetAchievement(m_pSteamUserStats, achievementId.c_str(), &achieved)) {
        return achieved;
    }
    return false;
}

bool SteamController::storeStats() {
    if (!m_initialized || !m_pSteamUserStats || !g_fnStoreStats) return false;
    return g_fnStoreStats(m_pSteamUserStats);
}

void SteamController::activateOverlay(const std::string& dialog) {
    if (!m_initialized || !m_pSteamFriends || !g_fnActivateGameOverlay) return;

    const char* dlg = dialog.empty() ? "community" : dialog.c_str();
    g_fnActivateGameOverlay(m_pSteamFriends, dlg);
    log("Steam: Activated Steam Overlay with dialog: '%s'", dlg);
}

void SteamController::triggerGameWinAchievement() {
    if (m_autoUnlockedHackMe) return;
    m_autoUnlockedHackMe = true;

    log("Steam: Player won the game! Unlocking achievement '%s' for AppID %u...",
        DEFAULT_ACHIEVEMENT_ID, m_appId);

    bool res = unlockAchievement(DEFAULT_ACHIEVEMENT_ID);

    // Notify in-game via RenderController banner toast
    std::string msg = res
        ? "[STEAM] Achievement Unlocked: HACK_ME! (AppID: 3832650)"
        : "[STEAM] Won Game! Granted HACK_ME for AppID: 3832650";

    RenderController::instance().showNotification(msg, 5.0f, Color(255, 215, 0));
}

} // namespace sense

