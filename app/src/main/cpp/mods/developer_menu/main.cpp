#include <api/sense_api.hpp>
#include "ui.hpp"

class DeveloperMenuMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Developer Menu";
        info.version = "1.0.0";
        info.author = "ModLoader";
        info.description = "Provides the F1 developer menu and registered mod controls.";
        return info;
    }

    void onEvent(const SDL_Event& event, bool& consumed) override {
        if (event.type == 0x300 && event.key.keysym.sym == sense::Key_F1 && event.key.repeat == 0) {
            m_menu.toggle();
            sense::ui::setCursorVisible(m_menu.isVisible());
            consumed = true;
            return;
        }

        if (m_menu.isVisible() && m_menu.handleEvent(event)) {
            sense::ui::setCursorVisible(m_menu.isVisible());
            consumed = true;
        }
    }

    void onInit() override {
        sense::events::onOverlayRender([this](SDL_Renderer* renderer, int screenW, int screenH) {
            m_menu.render(renderer, screenW, screenH);
        });
    }

private:
    DeveloperMenuUI m_menu;
};

REGISTER_MOD(DeveloperMenuMod)
