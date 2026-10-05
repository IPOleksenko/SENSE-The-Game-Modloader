#include <api/sense_api.hpp>

class TestGameObjectsMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Test: Game Objects Modification API";
        info.version = "1.0.0";
        info.author = "SenseTestSuite";
        info.description = "Test harness for intercepting, tinting, scaling, and modifying native in-game rendering sprites and textures.";
        return info;
    }

    void onInit() override {
        sense::log("TestGameObjectsMod: Initializing game objects modification test harness!");

        // F1 Menu Section
        sense::ui::addSection("API Test: In-Game Objects Modification");

        sense::ui::addButton("Apply Cyberpunk Purple Tint", [this]() {
            sense::gameobjects::clearModifiers();
            sense::gameobjects::setGlobalTint(sense::Color(200, 50, 255));
            sense::render::showNotification("Tint Applied: Cyberpunk Purple", 2.0f, sense::Color(200, 50, 255));
        });

        sense::ui::addButton("Apply Golden Glow Tint", [this]() {
            sense::gameobjects::clearModifiers();
            sense::gameobjects::setGlobalTint(sense::Color(255, 215, 0));
            sense::render::showNotification("Tint Applied: Golden Glow", 2.0f, sense::Color(255, 215, 0));
        });

        sense::ui::addButton("Scale Game Objects 1.5x (Giant Mode)", [this]() {
            sense::gameobjects::clearModifiers();
            sense::gameobjects::scaleObjects(1.5f, 1.5f);
            sense::render::showNotification("Game Objects Scaled: 1.5x", 2.0f, sense::Color::Cyan());
        });

        sense::ui::addButton("Scale Game Objects 0.7x (Mini Mode)", [this]() {
            sense::gameobjects::clearModifiers();
            sense::gameobjects::scaleObjects(0.7f, 0.7f);
            sense::render::showNotification("Game Objects Scaled: 0.7x", 2.0f, sense::Color::Yellow());
        });

        sense::ui::addButton("Invert / Flip Vertically", [this]() {
            sense::gameobjects::clearModifiers();
            sense::GameObjectModifier mod;
            mod.name = "Flip Vertically";
            mod.flip = 2; // SDL_FLIP_VERTICAL
            sense::gameobjects::addModifier(mod);
            sense::render::showNotification("Game Objects Flipped Vertically!", 2.0f, sense::Color::Red());
        });

        sense::ui::addButton("Reset All Object Modifications", [this]() {
            sense::gameobjects::clearModifiers();
            sense::render::showNotification("All Object Modifications Reset to Normal", 2.0f, sense::Color::Green());
        });
    }
};

REGISTER_MOD(TestGameObjectsMod)

