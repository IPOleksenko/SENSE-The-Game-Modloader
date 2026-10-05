#include "savestate.hpp"
#include "player.hpp"
#include "game.hpp"
#include "render.hpp"
#include "lowlevel.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace sense {

SaveStateController& SaveStateController::instance() {
    static SaveStateController inst;
    return inst;
}

static std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
    localtime_s(&buf, &in_time_t);
    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool SaveStateController::saveState(int slot, const std::string& tag) {
    auto& pc = PlayerController::instance();
    auto& gc = GameController::instance();

    if (!pc.isValid()) return false;

    SaveStateData state;
    state.valid = true;
    state.tag = tag.empty() ? ("Slot " + std::to_string(slot)) : tag;
    state.timestamp = getCurrentTimeString();

    state.posY = pc.getY();
    state.speed = pc.getSpeed();
    state.isMoving = pc.isMoving();
    state.hasLost = pc.hasLost();
    state.speedScale = pc.getSpeedScale();
    state.speedDecrease = pc.getSpeedDecrease();
    state.currentFrame = pc.getCurrentFrame();
    state.lastMove = pc.getLastMove();
    state.balance = pc.getBalance();

    state.endlessMode = gc.isEndlessMode();
    state.timeScale = gc.getTimeScale();

    state.customData = m_pendingCustomData;

    m_slots[slot] = state;
    return true;
}

bool SaveStateController::loadState(int slot) {
    auto it = m_slots.find(slot);
    if (it == m_slots.end() || !it->second.valid) {
        return false;
    }

    const auto& s = it->second;
    auto& pc = PlayerController::instance();
    auto& gc = GameController::instance();

    if (!pc.isValid()) return false;

    // Restore player state
    pc.teleport(s.posY);
    pc.setSpeed(s.speed);
    pc.setMoving(s.isMoving);
    pc.setSpeedScale(s.speedScale);
    pc.setSpeedDecrease(s.speedDecrease);
    pc.setCurrentFrame(s.currentFrame);
    pc.setLastMove(s.lastMove);

    if (!s.hasLost && pc.hasLost()) {
        pc.resetDefeat();
    }

    // Restore game state
    gc.setEndlessModeDirect(s.endlessMode);
    gc.setTimeScale(s.timeScale);

    m_pendingCustomData = s.customData;
    return true;
}

bool SaveStateController::hasState(int slot) const {
    auto it = m_slots.find(slot);
    return it != m_slots.end() && it->second.valid;
}

const SaveStateData* SaveStateController::getStateData(int slot) const {
    auto it = m_slots.find(slot);
    return (it != m_slots.end() && it->second.valid) ? &it->second : nullptr;
}

void SaveStateController::clearState(int slot) {
    m_slots.erase(slot);
}

bool SaveStateController::quickSave() {
    bool ok = saveState(0, "QuickSave");
    if (ok) {
        RenderController::instance().showNotification("QuickSave Saved! (Slot 0)", 2.0f, Color::Green());
    }
    return ok;
}

bool SaveStateController::quickLoad() {
    bool ok = loadState(0);
    if (ok) {
        RenderController::instance().showNotification("QuickSave Loaded! (Slot 0)", 2.0f, Color::Cyan());
    } else {
        RenderController::instance().showNotification("No QuickSave found!", 2.0f, Color::Red());
    }
    return ok;
}

bool SaveStateController::saveToFile(int slot, const std::string& filePath) {
    if (!hasState(slot)) {
        if (!saveState(slot)) return false;
    }

    const auto& s = m_slots[slot];
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "[SaveState]\n";
    out << "tag=" << s.tag << "\n";
    out << "timestamp=" << s.timestamp << "\n";
    out << "posY=" << s.posY << "\n";
    out << "speed=" << s.speed << "\n";
    out << "isMoving=" << (s.isMoving ? 1 : 0) << "\n";
    out << "hasLost=" << (s.hasLost ? 1 : 0) << "\n";
    out << "speedScale=" << s.speedScale << "\n";
    out << "speedDecrease=" << s.speedDecrease << "\n";
    out << "currentFrame=" << s.currentFrame << "\n";
    out << "lastMove=" << static_cast<int>(s.lastMove) << "\n";
    out << "balance=" << s.balance << "\n";
    out << "endlessMode=" << (s.endlessMode ? 1 : 0) << "\n";
    out << "timeScale=" << s.timeScale << "\n";

    out << "[CustomData]\n";
    for (const auto& [k, v] : s.customData) {
        out << k << "=" << v << "\n";
    }

    return true;
}

bool SaveStateController::loadFromFile(int slot, const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    SaveStateData s;
    s.valid = true;

    std::string line;
    bool inCustom = false;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line == "[CustomData]") {
            inCustom = true;
            continue;
        }
        if (line == "[SaveState]") {
            inCustom = false;
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);

        if (inCustom) {
            s.customData[key] = val;
        } else {
            if (key == "tag") s.tag = val;
            else if (key == "timestamp") s.timestamp = val;
            else if (key == "posY") s.posY = std::stoi(val);
            else if (key == "speed") s.speed = std::stof(val);
            else if (key == "isMoving") s.isMoving = (val == "1");
            else if (key == "hasLost") s.hasLost = (val == "1");
            else if (key == "speedScale") s.speedScale = std::stof(val);
            else if (key == "speedDecrease") s.speedDecrease = std::stof(val);
            else if (key == "currentFrame") s.currentFrame = std::stoi(val);
            else if (key == "lastMove") s.lastMove = static_cast<PlayerMove>(std::stoi(val));
            else if (key == "balance") s.balance = std::stof(val);
            else if (key == "endlessMode") s.endlessMode = (val == "1");
            else if (key == "timeScale") s.timeScale = std::stof(val);
        }
    }

    m_slots[slot] = s;
    return true;
}

void SaveStateController::setCustomData(const std::string& key, const std::string& value) {
    m_pendingCustomData[key] = value;
}

std::string SaveStateController::getCustomData(const std::string& key, const std::string& defaultVal) const {
    auto it = m_pendingCustomData.find(key);
    return (it != m_pendingCustomData.end()) ? it->second : defaultVal;
}

void SaveStateController::setAutoSaveOnCheckpoint(bool enabled) {
    m_autoSaveCheckpoint = enabled;
}

bool SaveStateController::isAutoSaveOnCheckpoint() const {
    return m_autoSaveCheckpoint;
}

void SaveStateController::onCheckpointReached(CheckPoint cp) {
    if (m_autoSaveCheckpoint) {
        saveState(1, "Checkpoint AutoSave");
        RenderController::instance().showNotification("Checkpoint AutoSaved! (Slot 1)", 1.5f, Color::Yellow());
    }
}

} // namespace sense

