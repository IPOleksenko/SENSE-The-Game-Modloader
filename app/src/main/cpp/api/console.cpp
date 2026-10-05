#include "console.hpp"
#include "render.hpp"
#include "player.hpp"
#include "game.hpp"
#include "environment.hpp"
#include <sstream>
#include <algorithm>

namespace sense {

ConsoleController& ConsoleController::instance() {
    static ConsoleController inst;
    return inst;
}

ConsoleController::ConsoleController() {
    print("SENSE Modloader Developer Console initialized. Type 'help' for commands.", Color(0, 220, 255));

    // Built-in commands
    registerCommand("help", "List all available console commands", [this](const std::vector<std::string>&) {
        print("Available Commands:", Color::Yellow());
        for (const auto& pair : m_commands) {
            print("  " + pair.first + " - " + pair.second.description, Color(180, 200, 220));
        }
    });

    registerCommand("clear", "Clear console output history", [this](const std::vector<std::string>&) {
        clear();
    });

    registerCommand("echo", "Print message to console", [this](const std::vector<std::string>& args) {
        std::string msg;
        for (size_t i = 1; i < args.size(); ++i) msg += args[i] + " ";
        print(msg, Color::White());
    });

    registerCommand("teleport", "Teleport player to Y position (usage: teleport <y>)", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            print("Usage: teleport <y>", Color::Red());
            return;
        }
        int y = std::stoi(args[1]);
        PlayerController::instance().teleport(y);
        print("Teleported player to Y = " + std::to_string(y), Color::Green());
    });

    registerCommand("speed", "Set player speed (usage: speed <value>)", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            print("Usage: speed <val>", Color::Red());
            return;
        }
        float spd = std::stof(args[1]);
        PlayerController::instance().setSpeed(spd);
        print("Player speed set to " + std::to_string(spd), Color::Green());
    });

    registerCommand("god", "Toggle God Mode (usage: god <0|1>)", [this](const std::vector<std::string>& args) {
        bool en = (args.size() >= 2) ? (args[1] == "1" || args[1] == "true") : !PlayerController::instance().isGodMode();
        PlayerController::instance().setGodMode(en);
        print(std::string("God Mode: ") + (en ? "ENABLED" : "DISABLED"), Color::Green());
    });

    registerCommand("timescale", "Set engine timescale (usage: timescale <value>)", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            print("Usage: timescale <value>", Color::Red());
            return;
        }
        float ts = std::stof(args[1]);
        GameController::instance().setTimeScale(ts);
        print("Timescale set to " + std::to_string(ts), Color::Green());
    });

    registerCommand("weather", "Set weather (usage: weather <none|rain|snow|fog|ash|matrix>)", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            print("Usage: weather <none|rain|snow|fog|ash|matrix>", Color::Red());
            return;
        }
        std::string w = args[1];
        if (w == "rain") EnvironmentController::instance().setWeather(WeatherType::Rain);
        else if (w == "snow") EnvironmentController::instance().setWeather(WeatherType::Snow);
        else if (w == "fog") EnvironmentController::instance().setWeather(WeatherType::Fog);
        else if (w == "ash") EnvironmentController::instance().setWeather(WeatherType::Ash);
        else if (w == "matrix") EnvironmentController::instance().setWeather(WeatherType::GlitchMatrix);
        else EnvironmentController::instance().setWeather(WeatherType::None);
        print("Weather updated to " + w, Color::Cyan());
    });
}

void ConsoleController::registerCommand(const std::string& name, const std::string& description, std::function<void(const std::vector<std::string>&)> handler) {
    ConsoleCommand cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.handler = handler;
    m_commands[name] = cmd;
}

void ConsoleController::print(const std::string& text, const Color& color) {
    ConsoleLogEntry entry;
    entry.text = text;
    entry.color = color;
    m_history.push_back(entry);
    if (m_history.size() > 100) {
        m_history.pop_front();
    }
}

void ConsoleController::clear() {
    m_history.clear();
}

void ConsoleController::execute(const std::string& commandLine) {
    if (commandLine.empty()) return;

    print("> " + commandLine, Color(0, 180, 255));
    m_commandHistory.push_back(commandLine);
    m_historyIndex = -1;

    std::stringstream ss(commandLine);
    std::string token;
    std::vector<std::string> args;
    while (ss >> token) {
        args.push_back(token);
    }

    if (args.empty()) return;

    std::string cmdName = args[0];
    auto it = m_commands.find(cmdName);
    if (it != m_commands.end()) {
        try {
            it->second.handler(args);
        } catch (const std::exception& e) {
            print(std::string("Error executing command: ") + e.what(), Color::Red());
        }
    } else {
        print("Unknown command: '" + cmdName + "'. Type 'help' for a list of commands.", Color::Red());
    }
}

void ConsoleController::setCVar(const std::string& name, const std::string& value) {
    m_cvars[name] = value;
}

std::string ConsoleController::getCVarString(const std::string& name, const std::string& defaultVal) const {
    auto it = m_cvars.find(name);
    return (it != m_cvars.end()) ? it->second : defaultVal;
}

float ConsoleController::getCVarFloat(const std::string& name, float defaultVal) const {
    auto it = m_cvars.find(name);
    if (it != m_cvars.end()) {
        try { return std::stof(it->second); } catch (...) {}
    }
    return defaultVal;
}

int ConsoleController::getCVarInt(const std::string& name, int defaultVal) const {
    auto it = m_cvars.find(name);
    if (it != m_cvars.end()) {
        try { return std::stoi(it->second); } catch (...) {}
    }
    return defaultVal;
}

bool ConsoleController::handleEvent(const SDL_Event& event) {
    if (event.type == 0x300 /* SDL_KEYDOWN */) {
        int key = event.key.keysym.sym;
        int scancode = event.key.keysym.scancode;

        // Toggle with ~ / ` (Grave / Tilde)
        if (scancode == 53 /* SDL_SCANCODE_GRAVE */ || key == '`' || key == '~') {
            toggle();
            return true;
        }

        if (!m_visible) return false;

        if (key == '\r' || key == '\n' || scancode == 40 /* SDL_SCANCODE_RETURN */) {
            execute(m_inputBuffer);
            m_inputBuffer.clear();
            return true;
        }

        if (key == '\b' || scancode == 42 /* SDL_SCANCODE_BACKSPACE */) {
            if (!m_inputBuffer.empty()) {
                m_inputBuffer.pop_back();
            }
            return true;
        }

        if (scancode == 43 /* SDL_SCANCODE_TAB */) {
            // Auto-complete
            if (!m_inputBuffer.empty()) {
                for (const auto& pair : m_commands) {
                    if (pair.first.rfind(m_inputBuffer, 0) == 0) {
                        m_inputBuffer = pair.first;
                        break;
                    }
                }
            }
            return true;
        }

        if (scancode == 82 /* SDL_SCANCODE_UP */) {
            if (!m_commandHistory.empty()) {
                if (m_historyIndex == -1) m_historyIndex = static_cast<int>(m_commandHistory.size()) - 1;
                else if (m_historyIndex > 0) m_historyIndex--;
                m_inputBuffer = m_commandHistory[m_historyIndex];
            }
            return true;
        }

        if (scancode == 81 /* SDL_SCANCODE_DOWN */) {
            if (!m_commandHistory.empty() && m_historyIndex != -1) {
                if (m_historyIndex < static_cast<int>(m_commandHistory.size()) - 1) {
                    m_historyIndex++;
                    m_inputBuffer = m_commandHistory[m_historyIndex];
                } else {
                    m_historyIndex = -1;
                    m_inputBuffer.clear();
                }
            }
            return true;
        }

        if (key >= 32 && key <= 126 && key != '`' && key != '~') {
            m_inputBuffer += static_cast<char>(key);
            return true;
        }

        return true; // Consume other keys while console is open
    }

    return m_visible;
}

void ConsoleController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_visible || !renderer) return;

    auto& rc = RenderController::instance();
    int consoleH = 300;

    // Background & Divider
    rc.fillRect(renderer, Rect(0, 0, screenW, consoleH), Color(10, 15, 24, 245));
    rc.drawLine(renderer, 0, consoleH, screenW, consoleH, Color(0, 200, 255, 255));
    rc.drawLine(renderer, 0, consoleH - 28, screenW, consoleH - 28, Color(50, 70, 100, 180));

    // Header
    rc.drawText(renderer, "=== SENSE DEVELOPER CONSOLE (Press ~ to toggle) ===", 12, 8, Color(0, 220, 255), 0.85f);

    // Logs (last 11 lines)
    int lineH = 18;
    int maxLines = (consoleH - 60) / lineH;
    int startIdx = (m_history.size() > static_cast<size_t>(maxLines)) ? static_cast<int>(m_history.size()) - maxLines : 0;
    int drawY = 32;

    for (size_t i = startIdx; i < m_history.size(); ++i) {
        rc.drawText(renderer, m_history[i].text, 14, drawY, m_history[i].color, 0.8f);
        drawY += lineH;
    }

    // Input prompt
    std::string prompt = "> " + m_inputBuffer + "_";
    rc.drawText(renderer, prompt, 14, consoleH - 22, Color::Yellow(), 0.9f);
}

} // namespace sense

