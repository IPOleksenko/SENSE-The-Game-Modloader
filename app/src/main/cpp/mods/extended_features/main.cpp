#include <api/sense_api.hpp>
#include <sstream>

class ExtendedFeaturesMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Advanced Subsystems & Modding API Showcase";
        info.version = "2.0.0";
        info.author = "SenseModTeam";
        info.description = "Showcases VFS, SaveStates, Replays/Ghosts, Physics, Particles, Weather, Dialogs, Speedrun splits, and Profiler.";
        return info;
    }

    void onInit() override {
        sense::log("ExtendedFeaturesMod: Initializing extended subsystems!");

        // 1. External DLL Loading Demo (Dynamic Native Interop)
        m_user32Lib.load("user32.dll");

        // 2. Hotkey 'O': QuickSave
        sense::input::registerHotkey(sense::Key_O, []() {
            sense::savestate::quickSave();
        });

        // 3. Hotkey 'P': QuickLoad
        sense::input::registerHotkey(sense::Key_P, []() {
            sense::savestate::quickLoad();
        });

        // 4. Hotkey F8: Camera 3D Perspective & Angle Cycle
        sense::input::registerHotkey(sense::Key_F8, [this]() {
            m_cameraMode = (m_cameraMode + 1) % 6;
            auto& cam = sense::api().camera();

            if (m_cameraMode == 0) {
                cam.reset();
                sense::render::showNotification("Camera: Normal 2D View [Default]", 2.0f, sense::Color::White());
            } else if (m_cameraMode == 1) {
                cam.preset3DHighway();
                sense::render::showNotification("Camera: 3D Highway (Mode-7 Perspective)", 2.0f, sense::Color::Green());
            } else if (m_cameraMode == 2) {
                cam.presetBirdEye();
                sense::render::showNotification("Camera: Bird's Eye (Top-Down View)", 2.0f, sense::Color::Cyan());
            } else if (m_cameraMode == 3) {
                cam.presetIsometric();
                sense::render::showNotification("Camera: Isometric 3D Perspective", 2.0f, sense::Color::Yellow());
            } else if (m_cameraMode == 4) {
                cam.reset();
                cam.setAngle(15.0f);
                cam.setZoom(1.25f);
                cam.shake(0.5f, 10.0f);
                sense::render::showNotification("Camera: Dynamic Action Roll [15° + 1.25x]", 2.0f, sense::Color::Magenta());
            } else if (m_cameraMode == 5) {
                cam.reset();
                cam.setAngle(180.0f);
                cam.setFlipped(true, false);
                sense::render::showNotification("Camera: Inverted Mirror Challenge Mode", 2.0f, sense::Color::Red());
            }
        });

        // 5. Hotkey F9: Post-Processing Visual Filters
        sense::input::registerHotkey(sense::Key_F9, [this]() {
            m_filterMode = (m_filterMode + 1) % 4;
            auto& cam = sense::api().camera();

            if (m_filterMode == 0) {
                cam.clearColorFilter();
                sense::render::showNotification("Visual Filter: Disabled", 2.0f, sense::Color::White());
            } else if (m_filterMode == 1) {
                cam.presetNightVision();
                sense::render::showNotification("Visual Filter: Night Vision", 2.0f, sense::Color::Green());
            } else if (m_filterMode == 2) {
                cam.presetCyberpunk();
                sense::render::showNotification("Visual Filter: Cyberpunk Neon", 2.0f, sense::Color::Magenta());
            } else if (m_filterMode == 3) {
                cam.presetSepia();
                sense::render::showNotification("Visual Filter: Sepia Tone", 2.0f, sense::Color::Yellow());
            }
        });

        // 6. Hotkey F10: System & Engine Profiler HUD Toggle
        sense::input::registerHotkey(sense::Key_F10, []() {
            sense::profiler::toggleOverlay();
        });

        // 7. Hotkey F11: Speedrun Splits HUD Toggle
        sense::input::registerHotkey(sense::Key_F11, []() {
            bool v = !sense::api().speedrun().isOverlayVisible();
            sense::api().speedrun().setOverlayVisible(v);
            sense::render::showNotification(v ? "Speedrun Splits: [ON]" : "Speedrun Splits: [OFF]", 1.5f);
        });

        // ====================================================================
        // Developer Menu Sections (F1)
        // ====================================================================

        // SaveStates Section
        sense::ui::addSection("SaveStates (O / P)");
        sense::ui::addButton("QuickSave (Slot 0)", []() {
            sense::savestate::quickSave();
        });
        sense::ui::addButton("QuickLoad (Slot 0)", []() {
            sense::savestate::quickLoad();
        });

        // Environment & Weather Section
        sense::ui::addSection("Weather & Atmospheric Lighting");
        sense::ui::addButton("Cycle Weather (Rain/Snow/Fog/Ash/Matrix)", [this]() {
            m_weatherMode = (m_weatherMode + 1) % 6;
            sense::WeatherType types[] = {
                sense::WeatherType::None,
                sense::WeatherType::Rain,
                sense::WeatherType::Snow,
                sense::WeatherType::Fog,
                sense::WeatherType::Ash,
                sense::WeatherType::GlitchMatrix
            };
            const char* names[] = { "None", "Heavy Rain", "Snowstorm", "Dense Fog", "Volcanic Ash", "Glitch Matrix" };
            sense::environment::setWeather(types[m_weatherMode]);
            sense::render::showNotification(std::string("Weather: ") + names[m_weatherMode], 2.0f, sense::Color::Cyan());
        });
        sense::ui::addButton("Cycle Day / Night Lighting", [this]() {
            m_timeOfDayMode = (m_timeOfDayMode + 1) % 4;
            float times[] = { 0.5f, 0.75f, 0.0f, 0.25f };
            const char* names[] = { "Day (Noon)", "Sunset (Dusk)", "Midnight (Night)", "Sunrise (Dawn)" };
            sense::environment::setTimeOfDay(times[m_timeOfDayMode]);
            sense::render::showNotification(std::string("Lighting: ") + names[m_timeOfDayMode], 2.0f, sense::Color::Yellow());
        });
        sense::ui::addButton("Flash Lightning Effect", []() {
            sense::environment::flashLightning(0.35f, sense::Color::White());
        });

        // Particle VFX Section
        sense::ui::addSection("2D Particle Engine VFX");
        sense::ui::addButton("Burst Confetti (Screen Center)", []() {
            sense::particles::spawnBurst(640, 360, 45, sense::Color::Cyan());
            sense::particles::spawnBurst(640, 360, 30, sense::Color::Yellow());
        });
        sense::ui::addButton("Sparks Eruption", []() {
            sense::particles::spawnSparks(640, 420, 25, sense::Color::Yellow());
        });
        sense::ui::addButton("Speed Lines Surge", []() {
            sense::particles::spawnSpeedLines(1280, 720, 2.5f);
        });

        // Physics Modifiers Section
        sense::ui::addSection("Player Physics & Handling");
        sense::ui::addButton("Moon Gravity (Low Friction)", []() {
            sense::physics::applyPreset(sense::PhysicsPreset::MoonGravity);
        });
        sense::ui::addButton("Supercar Nitrous (High Traction)", []() {
            sense::physics::applyPreset(sense::PhysicsPreset::SupercarTraction);
        });
        sense::ui::addButton("Ice Road Drift (Slippery)", []() {
            sense::physics::applyPreset(sense::PhysicsPreset::IceRoad);
        });
        sense::ui::addButton("Reset Physics to Default", []() {
            sense::physics::reset();
            sense::render::showNotification("Physics Reset", 1.5f);
        });

        // Story & Dialog System Section
        sense::ui::addSection("Story & Dialog System");
        sense::ui::addButton("Show Comic Speech Bubble", []() {
            sense::dialog::showSpeechBubble("Racer", "Push through the checkpoint!", 640, 340, 3.0f);
        });
        sense::ui::addButton("Show RPG Story Dialog Box", []() {
            sense::dialog::showDialog("COACH", "Listen to me, rookie! Balance is everything in SENSE. Keep your rhythm steady and watch the speedometer!", []() {
                sense::render::showNotification("Dialog Completed!", 1.5f, sense::Color::Green());
            });
        });
        sense::ui::addButton("Show Cinematic Subtitle", []() {
            sense::dialog::showSubtitle(">> SECTION COMPLETE: RECORD PACE MAINTAINED <<", 3.0f);
        });

        // Performance Profiler & Speedrun Splits Section
        sense::ui::addSection("Performance & Speedrun Splits");
        sense::ui::addButton("Toggle Profiler HUD (F10)", []() {
            sense::profiler::toggleOverlay();
        });
        sense::ui::addButton("Toggle Speedrun Splits (F11)", []() {
            bool v = !sense::api().speedrun().isOverlayVisible();
            sense::api().speedrun().setOverlayVisible(v);
        });
        sense::ui::addButton("Reset Speedrun Splits", []() {
            sense::api().speedrun().reset();
            sense::render::showNotification("Speedrun Timer Reset", 1.5f);
        });

        // Network Telemetry
        sense::ui::addSection("Network & External Systems");
        sense::ui::addButton("Send HTTP Telemetry (GET)", []() {
            sense::net::getAsync("https://httpbin.org/get", [](const sense::HttpResponse& res) {
                sense::render::showNotification(res.success ? "HTTP 200 OK!" : "HTTP Error", 2.5f, res.success ? sense::Color::Green() : sense::Color::Red());
            });
        });
    }

    void onUpdate(float deltaTime) override {
        // High speed line effect when player goes fast
        auto& pc = sense::api().player();
        if (pc.isValid() && pc.isMoving() && pc.getSpeed() > 135.0f) {
            sense::particles::spawnSpeedLines(1280, 720, 0.5f);
        }
    }

private:
    sense::NativeLibrary m_user32Lib;
    int m_cameraMode = 0;
    int m_filterMode = 0;
    int m_weatherMode = 0;
    int m_timeOfDayMode = 0;
};

REGISTER_MOD(ExtendedFeaturesMod)
