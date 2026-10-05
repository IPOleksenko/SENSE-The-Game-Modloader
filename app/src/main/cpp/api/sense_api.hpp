#pragma once

#include "types.hpp"
#include "lowlevel.hpp"
#include "plugin.hpp"
#include "net.hpp"
#include "camera.hpp"
#include "player.hpp"
#include "game.hpp"
#include "render.hpp"
#include "input.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "events.hpp"
#include "mod.hpp"

// Extended Subsystems
#include "assets.hpp"
#include "savestate.hpp"
#include "replay.hpp"
#include "physics.hpp"
#include "particles.hpp"
#include "environment.hpp"
#include "dialog.hpp"
#include "speedrun.hpp"
#include "profiler.hpp"
#include "achievements.hpp"
#include "console.hpp"
#include "track.hpp"
#include "photomode.hpp"
#include "objects.hpp"
#include "gameobjects.hpp"
#include "steam.hpp"

#include <iostream>
#include <cstdio>
#include <cstdarg>

namespace sense {

// Unified logging utility
inline void log(const char* format, ...) {
    char buffer[1024] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
    std::cout << "[SenseAPI] " << buffer << std::endl;
}

// Master API Hub for object-oriented style
class SenseApiHub {
public:
    static SenseApiHub& instance() {
        static SenseApiHub hub;
        return hub;
    }

    PlayerController& player() { return PlayerController::instance(); }
    GameController& game() { return GameController::instance(); }
    CameraController& camera() { return CameraController::instance(); }
    RenderController& render() { return RenderController::instance(); }
    InputManager& input() { return InputManager::instance(); }
    AudioController& audio() { return AudioController::instance(); }
    UIController& ui() { return UIController::instance(); }
    EventBus& events() { return EventBus::instance(); }
    PluginManager& plugins() { return PluginManager::instance(); }

    // Extended Subsystems
    AssetController& assets() { return AssetController::instance(); }
    SaveStateController& savestate() { return SaveStateController::instance(); }
    ReplayController& replay() { return ReplayController::instance(); }
    PhysicsController& physics() { return PhysicsController::instance(); }
    ParticleController& particles() { return ParticleController::instance(); }
    EnvironmentController& environment() { return EnvironmentController::instance(); }
    LocalizationController& localization() { return LocalizationController::instance(); }
    DialogController& dialog() { return DialogController::instance(); }
    SpeedrunController& speedrun() { return SpeedrunController::instance(); }
    ProfilerController& profiler() { return ProfilerController::instance(); }
    AchievementController& achievements() { return AchievementController::instance(); }
    ConsoleController& console() { return ConsoleController::instance(); }
    TrackController& track() { return TrackController::instance(); }
    PhotoModeController& photomode() { return PhotoModeController::instance(); }
    ObjectController& objects() { return ObjectController::instance(); }
    GameObjectsController& gameobjects() { return GameObjectsController::instance(); }
    SteamController& steam() { return SteamController::instance(); }

private:
    SenseApiHub() = default;
};

inline SenseApiHub& api() {
    return SenseApiHub::instance();
}

} // namespace sense

