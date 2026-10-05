#include "input.hpp"
#include "render.hpp"

namespace sense {

InputManager& InputManager::instance() {
    static InputManager inst;
    return inst;
}

bool InputManager::isKeyDown(KeyCode key) const {
    auto it = m_currentKeys.find(static_cast<int32_t>(key));
    return (it != m_currentKeys.end()) && it->second;
}

bool InputManager::isKeyPressed(KeyCode key) const {
    auto curIt = m_currentKeys.find(static_cast<int32_t>(key));
    bool cur = (curIt != m_currentKeys.end()) && curIt->second;

    auto prevIt = m_previousKeys.find(static_cast<int32_t>(key));
    bool prev = (prevIt != m_previousKeys.end()) && prevIt->second;

    return cur && !prev;
}

bool InputManager::isKeyReleased(KeyCode key) const {
    auto curIt = m_currentKeys.find(static_cast<int32_t>(key));
    bool cur = (curIt != m_currentKeys.end()) && curIt->second;

    auto prevIt = m_previousKeys.find(static_cast<int32_t>(key));
    bool prev = (prevIt != m_previousKeys.end()) && prevIt->second;

    return !cur && prev;
}

void InputManager::registerHotkey(KeyCode key, std::function<void()> callback) {
    m_hotkeys[key].push_back(callback);
}

void InputManager::registerToggle(KeyCode key, bool& toggleVariable, const std::string& notificationName) {
    registerHotkey(key, [&toggleVariable, notificationName]() {
        toggleVariable = !toggleVariable;
        if (!notificationName.empty()) {
            std::string status = notificationName + ": " + (toggleVariable ? "[ENABLED]" : "[DISABLED]");
            Color col = toggleVariable ? Color::Green() : Color::Red();
            RenderController::instance().showNotification(status, 2.0f, col);
        }
    });
}

Point InputManager::getMousePos() const {
    return m_mousePos;
}

bool InputManager::isMouseButtonDown(int button) const {
    if (button >= 1 && button <= 3) {
        return m_currentMouseButtons[button];
    }
    return false;
}

bool InputManager::isMouseButtonClicked(int button) const {
    if (button >= 1 && button <= 3) {
        return m_currentMouseButtons[button] && !m_previousMouseButtons[button];
    }
    return false;
}

bool InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case 0x300: // SDL_KEYDOWN
        {
            int32_t sym = event.key.keysym.sym;
            m_currentKeys[sym] = true;

            if (event.key.repeat == 0) {
                auto it = m_hotkeys.find(static_cast<KeyCode>(sym));
                if (it != m_hotkeys.end()) {
                    for (auto& cb : it->second) {
                        cb();
                    }
                }
            }
            break;
        }
        case 0x301: // SDL_KEYUP
        {
            int32_t sym = event.key.keysym.sym;
            m_currentKeys[sym] = false;
            break;
        }
        case 0x400: // SDL_MOUSEMOTION
        {
            m_mousePos = Point(event.motion.x, event.motion.y);
            break;
        }
        case 0x401: // SDL_MOUSEBUTTONDOWN
        {
            if (event.button.button <= 3) {
                m_currentMouseButtons[event.button.button] = true;
            }
            m_mousePos = Point(event.button.x, event.button.y);
            break;
        }
        case 0x402: // SDL_MOUSEBUTTONUP
        {
            if (event.button.button <= 3) {
                m_currentMouseButtons[event.button.button] = false;
            }
            m_mousePos = Point(event.button.x, event.button.y);
            break;
        }
        default:
            break;
    }
    return false;
}

void InputManager::newFrame() {
    m_previousKeys = m_currentKeys;
    for (int i = 0; i < 4; ++i) {
        m_previousMouseButtons[i] = m_currentMouseButtons[i];
    }
}

} // namespace sense

