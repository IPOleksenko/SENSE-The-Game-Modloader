#include "ui.hpp"
namespace sense {

UIController::UIController() = default;

UIController& UIController::instance() {
    static UIController inst;
    return inst;
}

void UIController::addButton(const std::string& label, std::function<void()> onClick) {
    UIWidget widget;
    widget.type = UIWidgetType::Button;
    widget.label = label;
    widget.onClick = onClick;
    m_modWidgets.push_back(widget);
}

void UIController::addCheckbox(const std::string& label, bool* boolValue) {
    UIWidget widget;
    widget.type = UIWidgetType::Checkbox;
    widget.label = label;
    widget.boolValue = boolValue;
    m_modWidgets.push_back(widget);
}

void UIController::addSlider(const std::string& label, float* floatValue, float minVal, float maxVal, float step) {
    UIWidget widget;
    widget.type = UIWidgetType::Slider;
    widget.label = label;
    widget.floatValue = floatValue;
    widget.minVal = minVal;
    widget.maxVal = maxVal;
    widget.step = step;
    m_modWidgets.push_back(widget);
}

void UIController::addLabel(const std::string& text) {
    UIWidget widget;
    widget.type = UIWidgetType::Label;
    widget.label = text;
    m_modWidgets.push_back(widget);
}

void UIController::addSection(const std::string& title) {
    UIWidget widget;
    widget.type = UIWidgetType::Section;
    widget.label = title;
    m_modWidgets.push_back(widget);
}

const std::vector<UIWidget>& UIController::getWidgets() const {
    return m_modWidgets;
}

void UIController::setCursorVisible(bool visible) {
    m_cursorVisible = visible;
}

bool UIController::isCursorVisible() const {
    return m_cursorVisible;
}

} // namespace sense
