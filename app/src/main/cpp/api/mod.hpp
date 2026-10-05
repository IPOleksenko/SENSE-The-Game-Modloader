#pragma once

#include "types.hpp"
#include "events.hpp"
#include <string>
#include <memory>
#include <vector>

namespace sense {

struct ModInfo {
    std::string name = "Unnamed Mod";
    std::string version = "1.0.0";
    std::string author = "Unknown";
    std::string description = "";
};

class IMod {
public:
    virtual ~IMod() = default;

    [[nodiscard]] virtual ModInfo getInfo() const = 0;

    // Lifecycle Callbacks
    virtual void onInit() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onPreRender(SDL_Renderer* renderer) {}
    virtual void onRender(SDL_Renderer* renderer) {}
    virtual void onEvent(const SDL_Event& event, bool& consumed) {}
    virtual void onCheckpoint(CheckPoint checkpoint) {}
    virtual void onPlayerLost() {}
    virtual void onPlayerWin() {}
};

class ModManager {
public:
    static ModManager& instance();

    void registerMod(std::shared_ptr<IMod> mod);
    const std::vector<std::shared_ptr<IMod>>& getMods() const;

private:
    ModManager() = default;
    std::vector<std::shared_ptr<IMod>> m_mods;
};

} // namespace sense

// Macro to register mod class inside mod DLL
#define REGISTER_MOD(ModClass) \
    static std::shared_ptr<ModClass> g_modInstance = nullptr; \
    extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
        if (fdwReason == DLL_PROCESS_ATTACH) { \
            DisableThreadLibraryCalls(hinstDLL); \
            g_modInstance = std::make_shared<ModClass>(); \
            sense::ModManager::instance().registerMod(g_modInstance); \
        } \
        return TRUE; \
    }

