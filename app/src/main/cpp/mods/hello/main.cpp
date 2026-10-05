#include <api/sense_api.hpp>

class HelloMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Hello World Mod";
        info.version = "1.0.0";
        info.author = "Modder";
        info.description = "Demonstrates SenseModAPI initialization, notifications, and custom HUD rendering.";
        return info;
    }

    void onInit() override {
        sense::log("HelloMod: Initializing!");
        sense::render::showNotification("Hello, World! SenseModAPI is active!", 4.0f, sense::Color::Green());

        // Register custom hotkey 'H' to show notification
        sense::input::registerHotkey(sense::Key_H, []() {
            sense::render::showNotification("You pressed H! Hello from HelloMod!", 2.5f, sense::Color::Yellow());
        });

        // Register custom UI button in Mod Menu
        sense::ui::addButton("Greet Player", []() {
            sense::render::showNotification("Greetings from the In-Game Menu!", 3.0f, sense::Color::Cyan());
        });
    }

    void onRender(SDL_Renderer* renderer) override {
        // Draw small mod watermark in bottom-left
        sense::Point winSize = sense::game::getWindowSize();
        sense::render::drawText(renderer, "[HelloMod Active - Press H]", 10, winSize.y - 20, sense::Color(100, 200, 255, 180), 1.0f);
    }
};

REGISTER_MOD(HelloMod)