#include "sense_api.hpp"
#include <intrin.h>
#include <chrono>
#include <mutex>
#include <atomic>

namespace sense {

// Synthetic event injection for Endless Mode
static std::atomic<int> g_pendingSpaceInject{0}; // 0 = none, 1 = KEYDOWN, 2 = KEYUP
static bool g_restoreMovingAfterSpace = false;

void requestEndlessModeToggle() {
    auto& pc = PlayerController::instance();
    if (pc.isValid() && pc.isMoving()) {
        g_restoreMovingAfterSpace = true;
        pc.setMoving(false);
    }
    g_pendingSpaceInject.store(1);
}

// ============================================================================
// EventBus Implementation
// ============================================================================
EventBus& EventBus::instance() {
    static EventBus bus;
    return bus;
}

void EventBus::subscribeInit(std::function<void()> handler) { m_initHandlers.push_back(handler); }
void EventBus::subscribeUpdate(std::function<void(float)> handler) { m_updateHandlers.push_back(handler); }
void EventBus::subscribePreRender(std::function<void(SDL_Renderer*)> handler) { m_preRenderHandlers.push_back(handler); }
void EventBus::subscribeRender(std::function<void(SDL_Renderer*)> handler) { m_renderHandlers.push_back(handler); }
void EventBus::subscribeEvent(std::function<void(const SDL_Event&, bool&)> handler) { m_eventHandlers.push_back(handler); }
void EventBus::subscribeCheckpoint(std::function<void(CheckPoint)> handler) { m_checkpointHandlers.push_back(handler); }
void EventBus::subscribePlayerLost(std::function<void()> handler) { m_lostHandlers.push_back(handler); }
void EventBus::subscribePlayerWin(std::function<void()> handler) { m_winHandlers.push_back(handler); }

void EventBus::dispatchInit() {
    for (auto& h : m_initHandlers) h();
}

void EventBus::dispatchUpdate(float deltaTime) {
    for (auto& h : m_updateHandlers) h(deltaTime);
}

void EventBus::dispatchPreRender(SDL_Renderer* renderer) {
    for (auto& h : m_preRenderHandlers) h(renderer);
}

void EventBus::dispatchRender(SDL_Renderer* renderer) {
    for (auto& h : m_renderHandlers) h(renderer);
}

bool EventBus::dispatchEvent(const SDL_Event& event) {
    bool consumed = false;
    for (auto& h : m_eventHandlers) {
        h(event, consumed);
        if (consumed) break;
    }
    return consumed;
}

void EventBus::dispatchCheckpoint(CheckPoint cp) {
    for (auto& h : m_checkpointHandlers) h(cp);
}

void EventBus::dispatchPlayerLost() {
    for (auto& h : m_lostHandlers) h();
}

void EventBus::dispatchPlayerWin() {
    for (auto& h : m_winHandlers) h();
}

// ============================================================================
// ModManager Implementation
// ============================================================================
void initCoreHooks();

ModManager& ModManager::instance() {
    static ModManager mgr;
    return mgr;
}

void ModManager::registerMod(std::shared_ptr<IMod> mod) {
    if (!mod) return;
    m_mods.push_back(mod);

    // Subscribe mod lifecycle methods to EventBus
    events::onInit([mod]() { mod->onInit(); });
    events::onUpdate([mod](float dt) { mod->onUpdate(dt); });
    events::onPreRender([mod](SDL_Renderer* r) { mod->onPreRender(r); });
    events::onRender([mod](SDL_Renderer* r) { mod->onRender(r); });
    events::onEvent([mod](const SDL_Event& ev, bool& c) { mod->onEvent(ev, c); });
    events::onCheckpoint([mod](CheckPoint cp) { mod->onCheckpoint(cp); });
    events::onPlayerLost([mod]() { mod->onPlayerLost(); });
    events::onPlayerWin([mod]() { mod->onPlayerWin(); });

    // Initialize core hooks
    initCoreHooks();
}

const std::vector<std::shared_ptr<IMod>>& ModManager::getMods() const {
    return m_mods;
}

// ============================================================================
// Hook Management & Stack Scanner
// ============================================================================
typedef void (*PFN_SDL_RenderPresent)(SDL_Renderer*);
typedef int  (*PFN_SDL_PollEvent)(SDL_Event*);
typedef void (*PFN_SDL_Delay)(uint32_t);
typedef int  (*PFN_SDL_RenderClear)(SDL_Renderer*);
typedef SDL_Window* (*PFN_SDL_RenderGetWindow)(SDL_Renderer*);
typedef void (*PFN_SDL_GetWindowSize)(SDL_Window*, int*, int*);
typedef void (*PFN_SDL_RenderWindowToLogical)(SDL_Renderer*, int, int, float*, float*);
typedef int  (*PFN_SDL_ShowCursor)(int);

typedef int  (*PFN_SDL_RenderCopy)(SDL_Renderer*, SDL_Texture*, const Rect*, const Rect*);
typedef int  (*PFN_SDL_RenderCopyEx)(SDL_Renderer*, SDL_Texture*, const Rect*, const Rect*, double, const Point*, int);
typedef int  (*PFN_SDL_SetTextureColorMod)(SDL_Texture*, uint8_t, uint8_t, uint8_t);
typedef int  (*PFN_SDL_GetTextureColorMod)(SDL_Texture*, uint8_t*, uint8_t*, uint8_t*);
typedef int  (*PFN_SDL_SetTextureAlphaMod)(SDL_Texture*, uint8_t);
typedef int  (*PFN_SDL_GetTextureAlphaMod)(SDL_Texture*, uint8_t*);

static PFN_SDL_RenderPresent g_orig_SDL_RenderPresent = nullptr;
static PFN_SDL_PollEvent     g_orig_SDL_PollEvent = nullptr;
static PFN_SDL_Delay         g_orig_SDL_Delay = nullptr;
static PFN_SDL_RenderClear   g_orig_SDL_RenderClear = nullptr;
static PFN_SDL_RenderGetWindow g_orig_SDL_RenderGetWindow = nullptr;
static PFN_SDL_GetWindowSize g_orig_SDL_GetWindowSize = nullptr;
static PFN_SDL_RenderWindowToLogical g_orig_SDL_RenderWindowToLogical = nullptr;
static PFN_SDL_ShowCursor    g_orig_SDL_ShowCursor = nullptr;

static PFN_SDL_RenderCopy   g_orig_SDL_RenderCopy = nullptr;
static PFN_SDL_RenderCopyEx g_orig_SDL_RenderCopyEx = nullptr;
static PFN_SDL_SetTextureColorMod g_fn_SDL_SetTextureColorMod = nullptr;
static PFN_SDL_GetTextureColorMod g_fn_SDL_GetTextureColorMod = nullptr;
static PFN_SDL_SetTextureAlphaMod g_fn_SDL_SetTextureAlphaMod = nullptr;
static PFN_SDL_GetTextureAlphaMod g_fn_SDL_GetTextureAlphaMod = nullptr;

static bool g_isMasterInstance = false;
static bool g_initDispatched = false;
static int  g_lastReportedCheckpointY = -1;

// IAT Hook Helper
static bool hookIat(HMODULE hModule, const char* targetDll, const char* funcName, void* newFunc, void** origFunc) {
    if (!hModule || !targetDll || !funcName || !newFunc) return false;

    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(hModule) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    auto importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0) return false;

    auto* importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<uint8_t*>(hModule) + importDir.VirtualAddress);

    for (; importDesc->Name != 0; ++importDesc) {
        const char* modName = reinterpret_cast<const char*>(reinterpret_cast<uint8_t*>(hModule) + importDesc->Name);
        if (_stricmp(modName, targetDll) == 0) {
            auto* thunkOrig = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<uint8_t*>(hModule) + importDesc->OriginalFirstThunk);
            auto* thunkIAT = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<uint8_t*>(hModule) + importDesc->FirstThunk);

            for (; thunkOrig->u1.Function != 0; ++thunkOrig, ++thunkIAT) {
                if (!(thunkOrig->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
                    auto* importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<uint8_t*>(hModule) + thunkOrig->u1.AddressOfData);
                    if (strcmp(importByName->Name, funcName) == 0) {
                        DWORD oldProtect = 0;
                        VirtualProtect(&thunkIAT->u1.Function, sizeof(uintptr_t), PAGE_READWRITE, &oldProtect);
                        if (origFunc && *origFunc == nullptr) {
                            *origFunc = reinterpret_cast<void*>(thunkIAT->u1.Function);
                        }
                        thunkIAT->u1.Function = reinterpret_cast<uintptr_t>(newFunc);
                        VirtualProtect(&thunkIAT->u1.Function, sizeof(uintptr_t), oldProtect, &oldProtect);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Stack Scanner to discover Player*
static void scanStackForPlayer() {
    auto& pc = PlayerController::instance();
    if (pc.isValid()) return;

    auto* tib = reinterpret_cast<PNT_TIB>(NtCurrentTeb());
    uintptr_t stackBase = reinterpret_cast<uintptr_t>(tib->StackBase);
    uintptr_t currentRsp = reinterpret_cast<uintptr_t>(_AddressOfReturnAddress());

    if (currentRsp >= stackBase) return;

    size_t scanLimit = std::min(static_cast<size_t>(stackBase - currentRsp), static_cast<size_t>(128 * 1024));

    for (size_t offset = 0; offset + 0x70 <= scanLimit; offset += sizeof(uintptr_t)) {
        uintptr_t candidate = currentRsp + offset;
        __try {
            float sMin = *reinterpret_cast<float*>(candidate + lowlevel::OFFSET_PLAYER_SPEED_MIN);
            float sMax = *reinterpret_cast<float*>(candidate + lowlevel::OFFSET_PLAYER_SPEED_MAX);
            float sNorm = *reinterpret_cast<float*>(candidate + lowlevel::OFFSET_PLAYER_SPEED_NORMAL);
            int frames = *reinterpret_cast<int*>(candidate + lowlevel::OFFSET_PLAYER_FRAMES_TOTAL);

            if (std::abs(sMin - 50.5f) < 0.01f &&
                std::abs(sMax - 150.0f) < 0.01f &&
                std::abs(sNorm - 100.25f) < 0.01f &&
                frames == 120) {
                pc.setRawInstance(reinterpret_cast<void*>(candidate));
                log("StackScanner: Discovered live Player* at 0x%p", reinterpret_cast<void*>(candidate));
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}

// ============================================================================
// Intercepted Functions
// ============================================================================
static void Hooked_SDL_RenderPresent(SDL_Renderer* renderer) {
    lowlevel::setRawRenderer(renderer);
    GameObjectsController::instance().onNewFrame();

    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    if (deltaTime > 0.2f) deltaTime = 0.016f;

    scanStackForPlayer();

    // Query logical renderer dimensions (SENSE uses 1280x720 canvas)
    int screenW = 1280;
    int screenH = 720;
    typedef void (*PFN_SDL_RenderGetLogicalSize)(SDL_Renderer*, int*, int*);
    static PFN_SDL_RenderGetLogicalSize fn_SDL_RenderGetLogicalSize = nullptr;
    if (!fn_SDL_RenderGetLogicalSize) {
        HMODULE hSdl = GetModuleHandleA("SDL2.dll");
        if (hSdl) {
            fn_SDL_RenderGetLogicalSize = reinterpret_cast<PFN_SDL_RenderGetLogicalSize>(GetProcAddress(hSdl, "SDL_RenderGetLogicalSize"));
        }
    }

    if (fn_SDL_RenderGetLogicalSize && renderer) {
        int logW = 0, logH = 0;
        fn_SDL_RenderGetLogicalSize(renderer, &logW, &logH);
        if (logW > 0 && logH > 0) {
            screenW = logW;
            screenH = logH;
        }
    }

    if (g_orig_SDL_RenderGetWindow && renderer) {
        SDL_Window* win = g_orig_SDL_RenderGetWindow(renderer);
        if (win) {
            lowlevel::setRawWindow(win);
        }
    }
    GameController::instance().setWindowSize(screenW, screenH);

    if (!g_initDispatched) {
        g_initDispatched = true;
        // Safely initialize Steam API for AppID 3832650 on the main game thread (outside DllMain)
        SteamController::instance().init(SteamController::DEFAULT_APP_ID);
        EventBus::instance().dispatchInit();
    }

    // Engine updates
    GameController::instance().update(deltaTime);
    GameController::instance().registerFrame();
    PlayerController::instance().update(deltaTime);
    CameraController::instance().update(deltaTime);
    PhysicsController::instance().update(deltaTime);
    ParticleController::instance().update(deltaTime);
    EnvironmentController::instance().update(deltaTime, screenW, screenH);
    DialogController::instance().update(deltaTime);
    ReplayController::instance().update(deltaTime);
    SpeedrunController::instance().update(deltaTime);
    ProfilerController::instance().recordFrame(deltaTime);
    TrackController::instance().update(deltaTime);
    AchievementController::instance().update(deltaTime);
    PhotoModeController::instance().update(deltaTime);
    ObjectController::instance().update(deltaTime);
    RenderController::instance().updateNotifications(deltaTime);
    SteamController::instance().update();
    EventBus::instance().dispatchUpdate(deltaTime);

    // Checkpoint, Victory & Defeat tracking
    static bool g_gameWonDispatched = false;
    auto& pc = PlayerController::instance();
    if (pc.isValid()) {
        int curY = pc.getY();
        if (curY != g_lastReportedCheckpointY) {
            g_lastReportedCheckpointY = curY;
            EventBus::instance().dispatchCheckpoint(static_cast<CheckPoint>(curY));
            SaveStateController::instance().onCheckpointReached(static_cast<CheckPoint>(curY));
        }
        if (pc.hasLost()) {
            EventBus::instance().dispatchPlayerLost();
            g_gameWonDispatched = false;
        }

        // Victory detection: reaching final checkpoint 25000+ or finishing final cutscene animation
        if (curY >= static_cast<int>(CheckPoint::FINAL_START) || pc.isFinalAnimationFinished()) {
            if (!g_gameWonDispatched) {
                g_gameWonDispatched = true;
                EventBus::instance().dispatchPlayerWin();
                // Achievement is granted by the steam_achievements plugin via onPlayerWin()
            }
        } else if (curY < 1000) {
            g_gameWonDispatched = false;
            SteamController::instance().resetWinTrigger();
        }
    }

    // 1. Present transformed camera scene (World Space -> Screen Space)
    CameraController::instance().presentScene(renderer, screenW, screenH);

    // Track Modifiers (Boost Pads, Oil Slicks, Road Markers)
    TrackController::instance().render(renderer, screenW, screenH);

    // 2D World Entities & 3D Objects (Billboards, Coins, Pyramids, Checkpoint Arches)
    ObjectController::instance().render(renderer, screenW, screenH);

    // Atmospheric Weather (on top of scene)
    EnvironmentController::instance().renderWeather(renderer, screenW, screenH);

    // Replay Ghost Racer
    ReplayController::instance().renderGhost(renderer, screenW, screenH);

    // 2D Particles (sparks, speed lines, bursts, smoke)
    ParticleController::instance().render(renderer);

    // Day / Night Ambient Lighting filter
    EnvironmentController::instance().renderLighting(renderer, screenW, screenH);

    // 2. Mod custom rendering in SCREEN space (Speedometer, AutoPilot, HUDs)
    EventBus::instance().dispatchRender(renderer);

    // Story Dialog & Speech Bubbles
    DialogController::instance().render(renderer, screenW, screenH);

    // Speedrun Splits Timer HUD
    SpeedrunController::instance().renderOverlay(renderer, screenW, screenH);

    // System & Engine Performance Profiler HUD
    ProfilerController::instance().renderOverlay(renderer, screenW, screenH);

    // Photo Mode Viewport & Controls
    PhotoModeController::instance().render(renderer, screenW, screenH);

    // Achievement Unlock Popups
    AchievementController::instance().render(renderer, screenW, screenH);

    // Quake Developer Terminal Console
    ConsoleController::instance().render(renderer, screenW, screenH);

    // 3. Master UI overlays in SCREEN space
    if (g_isMasterInstance) {
        UIController::instance().render(renderer, screenW, screenH);
    }
    RenderController::instance().renderNotifications(renderer, screenW, screenH);

    // 4. Dynamic VSync control for Speedhack & Slow-Mo
    // SENSE_THE_GAME initializes renderer with SDL_RENDERER_PRESENTVSYNC, which hard-caps
    // SDL_RenderPresent to 60 FPS (16.6ms). To allow speedhack (>1.0x) or slow-mo (<1.0x),
    // we disable VSync when timeScale != 1.0f!
    typedef int (*PFN_SDL_RenderSetVSync)(SDL_Renderer*, int);
    static PFN_SDL_RenderSetVSync fn_SDL_RenderSetVSync = nullptr;
    if (!fn_SDL_RenderSetVSync) {
        HMODULE hSdl = GetModuleHandleA("SDL2.dll");
        if (hSdl) {
            fn_SDL_RenderSetVSync = reinterpret_cast<PFN_SDL_RenderSetVSync>(GetProcAddress(hSdl, "SDL_RenderSetVSync"));
        }
    }

    float timeScale = GameController::instance().getTimeScale();
    if (fn_SDL_RenderSetVSync && renderer) {
        static int s_activeVSync = -1;
        int targetVSync = (std::abs(timeScale - 1.0f) < 0.05f) ? 1 : 0;
        if (s_activeVSync != targetVSync) {
            fn_SDL_RenderSetVSync(renderer, targetVSync);
            s_activeVSync = targetVSync;
        }
    }

    if (g_orig_SDL_RenderPresent) {
        g_orig_SDL_RenderPresent(renderer);
    }
}

static int Hooked_SDL_PollEvent(SDL_Event* event) {
    // 1. Process synthetic event injection for Endless Mode
    int injectPhase = g_pendingSpaceInject.load();
    if (injectPhase > 0 && event) {
        if (injectPhase == 1) {
            // Deliver KEYDOWN (Space) to Game::play()
            g_pendingSpaceInject.store(2);
            memset(event, 0, sizeof(SDL_Event));
            event->type = 0x300; // SDL_KEYDOWN
            event->key.type = 0x300;
            event->key.state = 1; // SDL_PRESSED
            event->key.repeat = 0;
            event->key.keysym.sym = ' '; // SDLK_SPACE
            event->key.keysym.scancode = 44; // SDL_SCANCODE_SPACE

            bool newMode = !GameController::instance().isEndlessMode();
            GameController::instance().setEndlessModeDirect(newMode);
            RenderController::instance().showNotification(
                newMode ? "Endless Mode: [ENABLED]" : "Endless Mode: [DISABLED]",
                2.0f,
                newMode ? Color::Green() : Color::Yellow()
            );

            return 1;
        } else if (injectPhase == 2) {
            // Deliver KEYUP (Space) to complete keystroke
            g_pendingSpaceInject.store(0);
            memset(event, 0, sizeof(SDL_Event));
            event->type = 0x301; // SDL_KEYUP
            event->key.type = 0x301;
            event->key.state = 0; // SDL_RELEASED
            event->key.repeat = 0;
            event->key.keysym.sym = ' '; // SDLK_SPACE
            event->key.keysym.scancode = 44; // SDL_SCANCODE_SPACE

            if (g_restoreMovingAfterSpace) {
                g_restoreMovingAfterSpace = false;
                auto& pc = PlayerController::instance();
                if (pc.isValid()) pc.setMoving(true);
            }

            return 1;
        }
    }

    if (!g_orig_SDL_PollEvent) return 0;

    int result = g_orig_SDL_PollEvent(event);
    if (result && event) {
        // Map window mouse coordinates to logical coordinates (crucial for fullscreen!)
        if (g_orig_SDL_RenderWindowToLogical && lowlevel::getRawRenderer()) {
            if (event->type == 0x400) { // SDL_MOUSEMOTION
                float lx = 0.0f, ly = 0.0f;
                g_orig_SDL_RenderWindowToLogical(lowlevel::getRawRenderer(), event->motion.x, event->motion.y, &lx, &ly);
                event->motion.x = static_cast<int32_t>(lx);
                event->motion.y = static_cast<int32_t>(ly);
            }
            else if (event->type == 0x401 || event->type == 0x402) { // SDL_MOUSEBUTTONDOWN / UP
                float lx = 0.0f, ly = 0.0f;
                g_orig_SDL_RenderWindowToLogical(lowlevel::getRawRenderer(), event->button.x, event->button.y, &lx, &ly);
                event->button.x = static_cast<int32_t>(lx);
                event->button.y = static_cast<int32_t>(ly);
            }
        }

        // Track physical keyboard/controller Endless Mode toggles in game
        if (event->type == 0x300 /* SDL_KEYDOWN */ && event->key.repeat == 0) {
            if (event->key.keysym.sym == ' ' /* SDLK_SPACE */) {
                if (!PlayerController::instance().isMoving()) {
                    bool newMode = !GameController::instance().isEndlessMode();
                    GameController::instance().setEndlessModeDirect(newMode);
                    RenderController::instance().showNotification(
                        newMode ? "Endless Mode: [ENABLED]" : "Endless Mode: [DISABLED]",
                        2.0f,
                        newMode ? Color::Green() : Color::Yellow()
                    );
                }
            }
        }
        else if (event->type == 0x653 /* SDL_CONTROLLERBUTTONDOWN */) {
            if (event->cbutton.button == 0 /* SDL_CONTROLLER_BUTTON_A */) {
                if (!PlayerController::instance().isMoving()) {
                    bool newMode = !GameController::instance().isEndlessMode();
                    GameController::instance().setEndlessModeDirect(newMode);
                    RenderController::instance().showNotification(
                        newMode ? "Endless Mode: [ENABLED]" : "Endless Mode: [DISABLED]",
                        2.0f,
                        newMode ? Color::Green() : Color::Yellow()
                    );
                }
            }
        }

        // 1. Quake Developer Console (intercepts first)
        if (ConsoleController::instance().handleEvent(*event)) {
            event->type = 0;
        }

        // 2. Photo Mode Controls
        if (event->type != 0 && PhotoModeController::instance().handleEvent(*event)) {
            event->type = 0;
        }

        InputManager::instance().handleEvent(*event);

        if (event->type != 0 && g_isMasterInstance) {
            if (UIController::instance().handleEvent(*event)) {
                // Event consumed by Developer Menu
                event->type = 0;
            }
        }

        bool consumed = EventBus::instance().dispatchEvent(*event);
        if (consumed) {
            event->type = 0;
        }
    } else {
        InputManager::instance().newFrame();
    }

    // Toggle system cursor visibility when menu or console is active
    if (g_orig_SDL_ShowCursor) {
        static bool lastCursor = false;
        bool menuOpen = UIController::instance().isVisible() ||
                        ConsoleController::instance().isVisible() ||
                        PhotoModeController::instance().isEnabled();
        if (menuOpen != lastCursor) {
            g_orig_SDL_ShowCursor(menuOpen ? 1 : 0);
            lastCursor = menuOpen;
        }
    }

    return result;
}

static void Hooked_SDL_Delay(uint32_t ms) {
    float timeScale = GameController::instance().getTimeScale();
    if (timeScale <= 0.01f) timeScale = 0.01f;

    uint32_t targetMs;
    if (std::abs(timeScale - 1.0f) < 0.05f) {
        // Normal speed: pass through as-is
        targetMs = ms;
    } else {
        // Scale the actual delay by timeScale:
        // >1x  -> shorter delay (faster game loop)
        // <1x  -> longer delay (slower game loop)
        float scaled = static_cast<float>(ms) / timeScale;
        if (scaled < 0.0f) scaled = 0.0f;
        targetMs = static_cast<uint32_t>(scaled);
        // At very high speed targetMs can be 0 — that's fine, just yields the CPU
    }

    if (g_orig_SDL_Delay) {
        g_orig_SDL_Delay(targetMs);
    }
}

static int Hooked_SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const Rect* srcrect, const Rect* dstrect) {
    if (!renderer || !texture) {
        return g_orig_SDL_RenderCopy ? g_orig_SDL_RenderCopy(renderer, texture, srcrect, dstrect) : 0;
    }

    static thread_local bool s_inCopy = false;
    if (s_inCopy) {
        return g_orig_SDL_RenderCopy ? g_orig_SDL_RenderCopy(renderer, texture, srcrect, dstrect) : 0;
    }
    struct Guard {
        bool& f;
        Guard(bool& flag) : f(flag) { f = true; }
        ~Guard() { f = false; }
    } guard(s_inCopy);

    Rect modifiedDst = dstrect ? *dstrect : Rect(0, 0, 1280, 720);
    const Rect* modifiedSrc = srcrect;
    SDL_Texture* targetTexture = texture;
    double angle = 0.0;
    int flip = 0;
    Color tint = Color::White();

    bool shouldRender = GameObjectsController::instance().interceptRenderCopy(
        renderer,
        targetTexture,
        modifiedSrc,
        modifiedDst,
        angle,
        flip,
        tint
    );

    if (!shouldRender) {
        return 0; // Object hidden / suppressed
    }

    uint8_t origR = 255, origG = 255, origB = 255, origA = 255;
    bool hadMod = false;
    if (tint.r != 255 || tint.g != 255 || tint.b != 255 || tint.a != 255) {
        if (g_fn_SDL_GetTextureColorMod) g_fn_SDL_GetTextureColorMod(targetTexture, &origR, &origG, &origB);
        if (g_fn_SDL_GetTextureAlphaMod) g_fn_SDL_GetTextureAlphaMod(targetTexture, &origA);
        if (g_fn_SDL_SetTextureColorMod) g_fn_SDL_SetTextureColorMod(targetTexture, tint.r, tint.g, tint.b);
        if (g_fn_SDL_SetTextureAlphaMod) g_fn_SDL_SetTextureAlphaMod(targetTexture, tint.a);
        hadMod = true;
    }

    int res = 0;
    if (angle != 0.0 || flip != 0) {
        if (g_orig_SDL_RenderCopyEx) {
            res = g_orig_SDL_RenderCopyEx(renderer, targetTexture, modifiedSrc, &modifiedDst, angle, nullptr, flip);
        } else if (g_orig_SDL_RenderCopy) {
            res = g_orig_SDL_RenderCopy(renderer, targetTexture, modifiedSrc, &modifiedDst);
        }
    } else {
        if (g_orig_SDL_RenderCopy) {
            res = g_orig_SDL_RenderCopy(renderer, targetTexture, modifiedSrc, &modifiedDst);
        }
    }

    if (hadMod) {
        if (g_fn_SDL_SetTextureColorMod) g_fn_SDL_SetTextureColorMod(targetTexture, origR, origG, origB);
        if (g_fn_SDL_SetTextureAlphaMod) g_fn_SDL_SetTextureAlphaMod(targetTexture, origA);
    }

    return res;
}

static int Hooked_SDL_RenderCopyEx(SDL_Renderer* renderer, SDL_Texture* texture, const Rect* srcrect, const Rect* dstrect, double angle, const Point* center, int flip) {
    if (!renderer || !texture) {
        return g_orig_SDL_RenderCopyEx ? g_orig_SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, angle, center, flip) : 0;
    }

    static thread_local bool s_inCopyEx = false;
    if (s_inCopyEx) {
        return g_orig_SDL_RenderCopyEx ? g_orig_SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, angle, center, flip) : 0;
    }
    struct GuardEx {
        bool& f;
        GuardEx(bool& flag) : f(flag) { f = true; }
        ~GuardEx() { f = false; }
    } guardEx(s_inCopyEx);

    Rect modifiedDst = dstrect ? *dstrect : Rect(0, 0, 1280, 720);
    const Rect* modifiedSrc = srcrect;
    SDL_Texture* targetTexture = texture;
    double finalAngle = angle;
    int finalFlip = flip;
    Color tint = Color::White();

    bool shouldRender = GameObjectsController::instance().interceptRenderCopy(
        renderer,
        targetTexture,
        modifiedSrc,
        modifiedDst,
        finalAngle,
        finalFlip,
        tint
    );

    if (!shouldRender) {
        return 0;
    }

    uint8_t origR = 255, origG = 255, origB = 255, origA = 255;
    bool hadMod = false;
    if (tint.r != 255 || tint.g != 255 || tint.b != 255 || tint.a != 255) {
        if (g_fn_SDL_GetTextureColorMod) g_fn_SDL_GetTextureColorMod(targetTexture, &origR, &origG, &origB);
        if (g_fn_SDL_GetTextureAlphaMod) g_fn_SDL_GetTextureAlphaMod(targetTexture, &origA);
        if (g_fn_SDL_SetTextureColorMod) g_fn_SDL_SetTextureColorMod(targetTexture, tint.r, tint.g, tint.b);
        if (g_fn_SDL_SetTextureAlphaMod) g_fn_SDL_SetTextureAlphaMod(targetTexture, tint.a);
        hadMod = true;
    }

    int res = 0;
    if (g_orig_SDL_RenderCopyEx) {
        res = g_orig_SDL_RenderCopyEx(renderer, targetTexture, modifiedSrc, &modifiedDst, finalAngle, center, finalFlip);
    } else if (g_orig_SDL_RenderCopy) {
        res = g_orig_SDL_RenderCopy(renderer, targetTexture, modifiedSrc, &modifiedDst);
    }

    if (hadMod) {
        if (g_fn_SDL_SetTextureColorMod) g_fn_SDL_SetTextureColorMod(targetTexture, origR, origG, origB);
        if (g_fn_SDL_SetTextureAlphaMod) g_fn_SDL_SetTextureAlphaMod(targetTexture, origA);
    }

    return res;
}

static int Hooked_SDL_RenderClear(SDL_Renderer* renderer) {
    lowlevel::setRawRenderer(renderer);
    GameObjectsController::instance().onNewFrame();

    int curW = 1280;
    int curH = 720;
    typedef void (*PFN_SDL_RenderGetLogicalSize)(SDL_Renderer*, int*, int*);
    static PFN_SDL_RenderGetLogicalSize fn_SDL_RenderGetLogicalSizeClear = nullptr;
    if (!fn_SDL_RenderGetLogicalSizeClear) {
        HMODULE hSdl = GetModuleHandleA("SDL2.dll");
        if (hSdl) {
            fn_SDL_RenderGetLogicalSizeClear = reinterpret_cast<PFN_SDL_RenderGetLogicalSize>(GetProcAddress(hSdl, "SDL_RenderGetLogicalSize"));
        }
    }
    if (fn_SDL_RenderGetLogicalSizeClear && renderer) {
        int logW = 0, logH = 0;
        fn_SDL_RenderGetLogicalSizeClear(renderer, &logW, &logH);
        if (logW > 0 && logH > 0) {
            curW = logW;
            curH = logH;
        }
    }

    if (g_orig_SDL_RenderGetWindow && renderer) {
        SDL_Window* win = g_orig_SDL_RenderGetWindow(renderer);
        if (win) {
            lowlevel::setRawWindow(win);
        }
    }

    // 1. Redirect to camera scene target texture if camera active
    CameraController::instance().beginScene(renderer, curW, curH);

    // 2. Clear active target
    int res = g_orig_SDL_RenderClear ? g_orig_SDL_RenderClear(renderer) : 0;

    EventBus::instance().dispatchPreRender(renderer);
    return res;
}


void initCoreHooks() {
    static bool hooksInstalled = false;
    if (hooksInstalled) return;
    hooksInstalled = true;

    // Master instance lock
    HANDLE hMutex = CreateMutexA(nullptr, FALSE, "Local\\SenseAPI_MasterInstance_Lock");
    if (hMutex && GetLastError() != ERROR_ALREADY_EXISTS) {
        g_isMasterInstance = true;
    }

    // Set 1ms multimedia timer resolution so Windows Sleep/SDL_Delay doesn't round up to 15.6ms!
    HMODULE hWinmm = LoadLibraryA("winmm.dll");
    if (hWinmm) {
        typedef UINT (WINAPI* PFN_timeBeginPeriod)(UINT);
        auto fnTimeBegin = reinterpret_cast<PFN_timeBeginPeriod>(GetProcAddress(hWinmm, "timeBeginPeriod"));
        if (fnTimeBegin) fnTimeBegin(1);
    }

    HMODULE hExe = GetModuleHandleA(nullptr);
    HMODULE hSdl = GetModuleHandleA("SDL2.dll");

    if (hSdl) {
        g_orig_SDL_RenderGetWindow = reinterpret_cast<PFN_SDL_RenderGetWindow>(GetProcAddress(hSdl, "SDL_RenderGetWindow"));
        g_orig_SDL_GetWindowSize = reinterpret_cast<PFN_SDL_GetWindowSize>(GetProcAddress(hSdl, "SDL_GetWindowSize"));
        g_orig_SDL_RenderWindowToLogical = reinterpret_cast<PFN_SDL_RenderWindowToLogical>(GetProcAddress(hSdl, "SDL_RenderWindowToLogical"));
        g_orig_SDL_ShowCursor = reinterpret_cast<PFN_SDL_ShowCursor>(GetProcAddress(hSdl, "SDL_ShowCursor"));

        g_orig_SDL_RenderCopy = reinterpret_cast<PFN_SDL_RenderCopy>(GetProcAddress(hSdl, "SDL_RenderCopy"));
        g_orig_SDL_RenderCopyEx = reinterpret_cast<PFN_SDL_RenderCopyEx>(GetProcAddress(hSdl, "SDL_RenderCopyEx"));
        g_fn_SDL_SetTextureColorMod = reinterpret_cast<PFN_SDL_SetTextureColorMod>(GetProcAddress(hSdl, "SDL_SetTextureColorMod"));
        g_fn_SDL_GetTextureColorMod = reinterpret_cast<PFN_SDL_GetTextureColorMod>(GetProcAddress(hSdl, "SDL_GetTextureColorMod"));
        g_fn_SDL_SetTextureAlphaMod = reinterpret_cast<PFN_SDL_SetTextureAlphaMod>(GetProcAddress(hSdl, "SDL_SetTextureAlphaMod"));
        g_fn_SDL_GetTextureAlphaMod = reinterpret_cast<PFN_SDL_GetTextureAlphaMod>(GetProcAddress(hSdl, "SDL_GetTextureAlphaMod"));

        g_orig_SDL_RenderPresent = reinterpret_cast<PFN_SDL_RenderPresent>(GetProcAddress(hSdl, "SDL_RenderPresent"));
        g_orig_SDL_PollEvent     = reinterpret_cast<PFN_SDL_PollEvent>(GetProcAddress(hSdl, "SDL_PollEvent"));
        g_orig_SDL_Delay         = reinterpret_cast<PFN_SDL_Delay>(GetProcAddress(hSdl, "SDL_Delay"));
        g_orig_SDL_RenderClear   = reinterpret_cast<PFN_SDL_RenderClear>(GetProcAddress(hSdl, "SDL_RenderClear"));

        hookIat(hExe, "SDL2.dll", "SDL_RenderCopy",    reinterpret_cast<void*>(Hooked_SDL_RenderCopy),    reinterpret_cast<void**>(&g_orig_SDL_RenderCopy));
        hookIat(hExe, "SDL2.dll", "SDL_RenderCopyEx",  reinterpret_cast<void*>(Hooked_SDL_RenderCopyEx),  reinterpret_cast<void**>(&g_orig_SDL_RenderCopyEx));
        hookIat(hExe, "SDL2.dll", "SDL_RenderPresent", reinterpret_cast<void*>(Hooked_SDL_RenderPresent), reinterpret_cast<void**>(&g_orig_SDL_RenderPresent));
        hookIat(hExe, "SDL2.dll", "SDL_PollEvent",     reinterpret_cast<void*>(Hooked_SDL_PollEvent),     reinterpret_cast<void**>(&g_orig_SDL_PollEvent));
        hookIat(hExe, "SDL2.dll", "SDL_Delay",         reinterpret_cast<void*>(Hooked_SDL_Delay),         reinterpret_cast<void**>(&g_orig_SDL_Delay));
        hookIat(hExe, "SDL2.dll", "SDL_RenderClear",   reinterpret_cast<void*>(Hooked_SDL_RenderClear),   reinterpret_cast<void**>(&g_orig_SDL_RenderClear));
    }

    log("Core: Hooks installed successfully. Master UI: %s", g_isMasterInstance ? "TRUE" : "FALSE");
}

} // namespace sense

