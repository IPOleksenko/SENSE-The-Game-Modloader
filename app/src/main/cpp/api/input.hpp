#pragma once

#include "types.hpp"
#include <unordered_map>
#include <functional>
#include <vector>

namespace sense {

class InputManager {
public:
    static InputManager& instance();

    // Key states
    bool isKeyDown(KeyCode key) const;
    bool isKeyPressed(KeyCode key) const;
    bool isKeyReleased(KeyCode key) const;

    // Hotkey Registration
    void registerHotkey(KeyCode key, std::function<void()> callback);
    void registerToggle(KeyCode key, bool& toggleVariable, const std::string& notificationName = "");

    // Mouse coordinates and buttons
    Point getMousePos() const;
    bool isMouseButtonDown(int button) const; // 1 = Left, 2 = Middle, 3 = Right
    bool isMouseButtonClicked(int button) const;

    // Event Handling
    bool handleEvent(const SDL_Event& event);
    void newFrame();

private:
    InputManager() = default;

    std::unordered_map<int32_t, bool> m_currentKeys;
    std::unordered_map<int32_t, bool> m_previousKeys;
    std::unordered_map<KeyCode, std::vector<std::function<void()>>> m_hotkeys;

    Point m_mousePos = { 0, 0 };
    bool m_currentMouseButtons[4] = { false, false, false, false };
    bool m_previousMouseButtons[4] = { false, false, false, false };
};

// Convenience namespace functions
namespace input {
    inline bool isKeyDown(KeyCode k) { return InputManager::instance().isKeyDown(k); }
    inline bool isKeyPressed(KeyCode k) { return InputManager::instance().isKeyPressed(k); }
    inline bool isKeyReleased(KeyCode k) { return InputManager::instance().isKeyReleased(k); }

    inline void registerHotkey(KeyCode k, std::function<void()> cb) { InputManager::instance().registerHotkey(k, cb); }
    inline void registerToggle(KeyCode k, bool& var, const std::string& name = "") {
        InputManager::instance().registerToggle(k, var, name);
    }

    inline Point getMousePos() { return InputManager::instance().getMousePos(); }
    inline bool isMouseButtonDown(int btn) { return InputManager::instance().isMouseButtonDown(btn); }
    inline bool isMouseButtonClicked(int btn) { return InputManager::instance().isMouseButtonClicked(btn); }
}

} // namespace sense

