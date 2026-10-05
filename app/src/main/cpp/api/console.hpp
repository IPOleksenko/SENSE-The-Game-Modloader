#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <functional>

namespace sense {

struct ConsoleCommand {
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>& args)> handler;
};

struct ConsoleLogEntry {
    std::string text;
    Color color = Color::White();
};

class ConsoleController {
public:
    static ConsoleController& instance();

    void registerCommand(const std::string& name, const std::string& description, std::function<void(const std::vector<std::string>& args)> handler);
    void execute(const std::string& commandLine);

    void print(const std::string& text, const Color& color = Color::White());
    void clear();

    void toggle() { m_visible = !m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    // CVar Subsystem
    void setCVar(const std::string& name, const std::string& value);
    std::string getCVarString(const std::string& name, const std::string& defaultVal = "") const;
    float getCVarFloat(const std::string& name, float defaultVal = 0.0f) const;
    int getCVarInt(const std::string& name, int defaultVal = 0) const;

    bool handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    ConsoleController();

    bool m_visible = false;
    std::string m_inputBuffer;
    size_t m_cursorPos = 0;

    std::map<std::string, ConsoleCommand> m_commands;
    std::map<std::string, std::string> m_cvars;
    std::deque<ConsoleLogEntry> m_history;
    std::vector<std::string> m_commandHistory;
    int m_historyIndex = -1;
};

namespace console {
    inline void registerCommand(const std::string& name, const std::string& desc, std::function<void(const std::vector<std::string>&)> h) {
        ConsoleController::instance().registerCommand(name, desc, h);
    }
    inline void execute(const std::string& cmd) { ConsoleController::instance().execute(cmd); }
    inline void print(const std::string& text, const Color& col = Color::White()) { ConsoleController::instance().print(text, col); }
    inline void toggle() { ConsoleController::instance().toggle(); }
    inline bool isVisible() { return ConsoleController::instance().isVisible(); }
}

} // namespace sense

