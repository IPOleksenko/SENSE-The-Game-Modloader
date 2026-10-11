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

    void addButton(const std::string& label, std::function<void()> onClick);
    void addCheckbox(const std::string& label, bool* boolValue);
    void addSlider(const std::string& label, float* floatValue, float minVal, float maxVal, float step = 0.1f);
    void addLabel(const std::string& text);
    void addSection(const std::string& title);
    const std::vector<UIWidget>& getWidgets() const;

    void setCursorVisible(bool visible);
    bool isCursorVisible() const;

private:
    UIController();
    bool m_cursorVisible = false;
    std::vector<UIWidget> m_modWidgets;
};

namespace ui {
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
    inline void setCursorVisible(bool visible) { UIController::instance().setCursorVisible(visible); }
    inline bool isCursorVisible() { return UIController::instance().isCursorVisible(); }
}

} // namespace sense
