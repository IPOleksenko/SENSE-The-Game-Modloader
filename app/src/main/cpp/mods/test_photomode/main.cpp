#include <api/sense_api.hpp>

class TestPhotoModeMod : public sense::IMod {
public:
    [[nodiscard]] sense::ModInfo getInfo() const override {
        sense::ModInfo info;
        info.name = "Test: Photo Mode & Screenshot Studio API";
        info.version = "1.0.0";
        info.author = "SenseTestSuite";
        info.description = "Test harness for free-cam Photo Mode, rule-of-thirds grid, and instant high-res BMP screenshot saving.";
        return info;
    }

    void onInit() override {
        sense::log("TestPhotoModeMod: Initializing Photo Mode test harness!");

        // Hotkey '4': Toggle Photo Mode
        sense::input::registerHotkey(sense::Key_4, []() {
            sense::photomode::toggle();
        });

        // Hotkey '5': Instant Screenshot
        sense::input::registerHotkey(sense::Key_5, [this]() {
            m_requestScreenshot = true;
        });

        // F1 Menu Section
        sense::ui::addSection("API Test: Photo Mode & Screenshots");
        sense::ui::addButton("Toggle Photo Mode (4)", []() {
            sense::photomode::toggle();
        });
        sense::ui::addButton("Capture Instant Screenshot (5)", [this]() {
            m_requestScreenshot = true;
        });
        sense::ui::addButton("Toggle Rule-of-Thirds Grid", []() {
            auto& pm = sense::api().photomode();
            pm.setGridVisible(!pm.isGridVisible());
            sense::render::showNotification(pm.isGridVisible() ? "Grid: [ON]" : "Grid: [OFF]", 1.5f);
        });
    }

    void onRender(SDL_Renderer* renderer) override {
        if (m_requestScreenshot && renderer) {
            m_requestScreenshot = false;
            sense::photomode::capture(renderer);
        }
    }

private:
    bool m_requestScreenshot = false;
};

REGISTER_MOD(TestPhotoModeMod)

