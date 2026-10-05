#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace sense {

// ============================================================================
// 1. Localization & String Override Controller
// ============================================================================
class LocalizationController {
public:
    static LocalizationController& instance();

    void overrideText(const std::string& key, const std::string& text);
    std::string getText(const std::string& key, const std::string& defaultVal = "") const;
    bool hasOverride(const std::string& key) const;
    void clearOverrides();
    void loadLanguageDictionary(const std::map<std::string, std::string>& dict);

private:
    LocalizationController() = default;
    std::map<std::string, std::string> m_overrides;
};

// ============================================================================
// 2. In-Game Dialog, Speech Bubble & Subtitle Controller
// ============================================================================
struct SpeechBubble {
    std::string speaker;
    std::string text;
    int x = 0;
    int y = 0;
    float duration = 3.0f;
    float timeRemaining = 3.0f;
    Color bubbleColor = Color(20, 25, 40, 230);
    Color textColor = Color::White();
};

class DialogController {
public:
    static DialogController& instance();

    // 1. Speech Bubbles (Anchored to world / player or screen)
    void showSpeechBubble(
        const std::string& speaker,
        const std::string& text,
        int x,
        int y,
        float durationSeconds = 3.0f,
        Color bubbleColor = Color(20, 25, 40, 230),
        Color textColor = Color::White()
    );

    // 2. RPG Story Dialog Box (Screen Bottom with Typewriter Effect)
    void showDialog(
        const std::string& speaker,
        const std::string& text,
        std::function<void()> onFinished = nullptr
    );
    void closeDialog();
    bool isDialogActive() const;
    void nextDialog(); // Advances or skips typewriter

    void setTypewriterSpeed(float charsPerSecond); // Default 40.0f

    // 3. Cinematic Subtitles
    void showSubtitle(const std::string& text, float durationSeconds = 3.5f);

    // 4. Engine Lifecycle
    void update(float deltaTime);
    void render(SDL_Renderer* renderer, int screenW, int screenH);

private:
    DialogController();

    std::vector<SpeechBubble> m_bubbles;

    // Active RPG Dialog state
    bool m_dialogActive = false;
    std::string m_dialogSpeaker;
    std::string m_dialogFullText;
    float m_dialogRevealedChars = 0.0f;
    float m_typewriterSpeed = 45.0f;
    std::function<void()> m_onDialogFinished = nullptr;

    // Subtitle state
    std::string m_subtitleText;
    float m_subtitleRemaining = 0.0f;
};

// Convenience namespace functions
namespace localization {
    inline void overrideText(const std::string& k, const std::string& txt) { LocalizationController::instance().overrideText(k, txt); }
    inline std::string getText(const std::string& k, const std::string& def = "") { return LocalizationController::instance().getText(k, def); }
    inline bool hasOverride(const std::string& k) { return LocalizationController::instance().hasOverride(k); }
}

namespace dialog {
    inline void showSpeechBubble(const std::string& spk, const std::string& txt, int x, int y, float dur = 3.0f) {
        DialogController::instance().showSpeechBubble(spk, txt, x, y, dur);
    }
    inline void showDialog(const std::string& spk, const std::string& txt, std::function<void()> cb = nullptr) {
        DialogController::instance().showDialog(spk, txt, cb);
    }
    inline void closeDialog() { DialogController::instance().closeDialog(); }
    inline bool isDialogActive() { return DialogController::instance().isDialogActive(); }
    inline void showSubtitle(const std::string& txt, float dur = 3.5f) { DialogController::instance().showSubtitle(txt, dur); }
}

} // namespace sense

