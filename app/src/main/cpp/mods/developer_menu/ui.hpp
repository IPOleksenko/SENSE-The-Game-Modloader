#pragma once

#include <api/ui.hpp>

class DeveloperMenuUI {
public:
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }
    bool handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    bool renderButton(
        SDL_Renderer* renderer,
        const sense::Rect& rect,
        const std::string& label,
        bool isActive = false,
        sense::Color activeColor = sense::Color(40, 160, 60),
        sense::Color normalColor = sense::Color(35, 45, 65)
    );
    bool renderCheckbox(SDL_Renderer* renderer, const sense::Rect& rect, const std::string& label, bool* value);
    bool renderSlider(
        SDL_Renderer* renderer,
        const sense::Rect& rect,
        const std::string& label,
        float* value,
        float minVal,
        float maxVal,
        float step = 0.1f
    );

    bool m_visible = false;
    int m_activeTab = 0;
    sense::Rect m_windowRect = { 80, 35, 580, 540 };
    bool m_dragging = false;
    sense::Point m_dragOffset = { 0, 0 };
    sense::Point m_mousePos = { 0, 0 };
    bool m_mouseDown = false;
    bool m_mouseClicked = false;
    sense::Rect m_contentViewport = { 90, 113, 560, 447 };
    bool m_contentHitTest = false;
    int m_contentScrollY = 0;
    int m_maxContentScrollY = 0;
    int m_modScrollY = 0;
    int m_maxModScrollY = 0;
    bool m_scrollDragging = false;
};
