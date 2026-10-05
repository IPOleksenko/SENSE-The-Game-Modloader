#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>

namespace sense {

enum class UIWidgetType {
    Button,
    Checkbox,
    Slider,
    Label,
    Section
};

struct UIWidget {
    UIWidgetType type;
    std::string label;
    std::function<void()> onClick;
    bool* boolValue = nullptr;
    float* floatValue = nullptr;
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float step = 0.1f;
};

class UIController {
public:
    static UIController& instance();

    bool isVisible() const;
    void setVisible(bool visible);
    void toggle();

    // Custom mod UI registration
    void addButton(const std::string& label, std::function<void()> onClick);
    void addCheckbox(const std::string& label, bool* boolValue);
    void addSlider(const std::string& label, float* floatValue, float minVal, float maxVal, float step = 0.1f);
    void addLabel(const std::string& text);
    void addSection(const std::string& title);

    // Immediate-mode UI Helpers
    bool renderButton(
        SDL_Renderer* renderer,
        const Rect& rect,
        const std::string& label,
        bool isActive = false,
        Color activeColor = Color(40, 160, 60),
        Color normalColor = Color(35, 45, 65)
    );
    bool renderCheckbox(SDL_Renderer* renderer, const Rect& rect, const std::string& label, bool* value);
    bool renderSlider(SDL_Renderer* renderer, const Rect& rect, const std::string& label, float* value, float minVal, float maxVal, float step = 0.1f);

    // Render & Event hooks
    void render(SDL_Renderer* renderer, int screenW, int screenH);
    bool handleEvent(const SDL_Event& event);

    Point getMousePos() const { return m_mousePos; }

private:
    UIController();
    bool m_visible = false;
    int m_activeTab = 0; // 0 = Dashboard, 1 = Cheats, 2 = Camera/FX, 3 = Teleport, 4 = Mods
    Rect m_windowRect = { 80, 35, 580, 540 };
    bool m_dragging = false;
    Point m_dragOffset = { 0, 0 };

    Point m_mousePos = { 0, 0 };
    bool m_mouseDown = false;
    bool m_mouseClicked = false;

    int m_modScrollY = 0;
    int m_maxModScrollY = 0;
    bool m_scrollDragging = false;

    std::vector<UIWidget> m_modWidgets;
};

// Convenience namespace functions
namespace ui {
    inline bool isVisible() { return UIController::instance().isVisible(); }
    inline void setVisible(bool v) { UIController::instance().setVisible(v); }
    inline void toggle() { UIController::instance().toggle(); }

    inline void addButton(const std::string& label, std::function<void()> onClick) {
        UIController::instance().addButton(label, onClick);
    }
    inline void addCheckbox(const std::string& label, bool* boolVal) {
        UIController::instance().addCheckbox(label, boolVal);
    }
    inline void addSlider(const std::string& label, float* floatVal, float minV, float maxV, float step = 0.1f) {
        UIController::instance().addSlider(label, floatVal, minV, maxV, step);
    }
    inline void addLabel(const std::string& text) {
        UIController::instance().addLabel(text);
    }
    inline void addSection(const std::string& title) {
        UIController::instance().addSection(title);
    }
}

} // namespace sense
