#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace sense {

struct SaveStateData {
    bool valid = false;
    std::string tag;
    std::string timestamp;

    // Player State
    int posY = 0;
    float speed = 0.0f;
    bool isMoving = false;
    bool hasLost = false;
    float speedScale = 5.0f;
    float speedDecrease = 0.5f;
    int currentFrame = 0;
    PlayerMove lastMove = PlayerMove::Undefined;
    float balance = 0.0f;

    // Game State
    bool endlessMode = false;
    float timeScale = 1.0f;

    // Custom mod variable storage
    std::map<std::string, std::string> customData;
};

class SaveStateController {
public:
    static SaveStateController& instance();

    // In-memory slot management (Slots 0 to 9; 0 = QuickSave)
    bool saveState(int slot = 0, const std::string& tag = "");
    bool loadState(int slot = 0);
    bool hasState(int slot = 0) const;
    const SaveStateData* getStateData(int slot = 0) const;
    void clearState(int slot = 0);

    // QuickSave / QuickLoad shortcuts
    bool quickSave();
    bool quickLoad();

    // File Persistence
    bool saveToFile(int slot, const std::string& filePath);
    bool loadFromFile(int slot, const std::string& filePath);

    // Custom mod data storage for next save state
    void setCustomData(const std::string& key, const std::string& value);
    std::string getCustomData(const std::string& key, const std::string& defaultVal = "") const;

    // Automatic Checkpoint Saving
    void setAutoSaveOnCheckpoint(bool enabled);
    bool isAutoSaveOnCheckpoint() const;
    void onCheckpointReached(CheckPoint cp);

private:
    SaveStateController() = default;

    std::map<int, SaveStateData> m_slots;
    std::map<std::string, std::string> m_pendingCustomData;
    bool m_autoSaveCheckpoint = false;
};

// Convenience namespace functions
namespace savestate {
    inline bool save(int slot = 0, const std::string& tag = "") { return SaveStateController::instance().saveState(slot, tag); }
    inline bool load(int slot = 0) { return SaveStateController::instance().loadState(slot); }
    inline bool has(int slot = 0) { return SaveStateController::instance().hasState(slot); }
    inline bool quickSave() { return SaveStateController::instance().quickSave(); }
    inline bool quickLoad() { return SaveStateController::instance().quickLoad(); }

    inline bool saveToFile(int slot, const std::string& path) { return SaveStateController::instance().saveToFile(slot, path); }
    inline bool loadFromFile(int slot, const std::string& path) { return SaveStateController::instance().loadFromFile(slot, path); }

    inline void setCustomData(const std::string& k, const std::string& v) { SaveStateController::instance().setCustomData(k, v); }
    inline std::string getCustomData(const std::string& k, const std::string& def = "") { return SaveStateController::instance().getCustomData(k, def); }

    inline void setAutoSaveOnCheckpoint(bool en) { SaveStateController::instance().setAutoSaveOnCheckpoint(en); }
    inline bool isAutoSaveOnCheckpoint() { return SaveStateController::instance().isAutoSaveOnCheckpoint(); }
}

} // namespace sense

