#include "dialog.hpp"
#include "render.hpp"
#include <algorithm>

namespace sense {

// ============================================================================
// LocalizationController Implementation
// ============================================================================
LocalizationController& LocalizationController::instance() {
    static LocalizationController inst;
    return inst;
}

void LocalizationController::overrideText(const std::string& key, const std::string& text) {
    m_overrides[key] = text;
}

std::string LocalizationController::getText(const std::string& key, const std::string& defaultVal) const {
    auto it = m_overrides.find(key);
    return (it != m_overrides.end()) ? it->second : defaultVal;
}

bool LocalizationController::hasOverride(const std::string& key) const {
    return m_overrides.find(key) != m_overrides.end();
}

void LocalizationController::clearOverrides() {
    m_overrides.clear();
}

void LocalizationController::loadLanguageDictionary(const std::map<std::string, std::string>& dict) {
    for (const auto& [k, v] : dict) {
        m_overrides[k] = v;
    }
}

// ============================================================================
// DialogController Implementation
// ============================================================================
DialogController::DialogController() = default;

DialogController& DialogController::instance() {
    static DialogController inst;
    return inst;
}

void DialogController::showSpeechBubble(
    const std::string& speaker,
    const std::string& text,
    int x,
    int y,
    float durationSeconds,
    Color bubbleColor,
    Color textColor
) {
    SpeechBubble b;
    b.speaker = speaker;
    b.text = text;
    b.x = x;
    b.y = y;
    b.duration = durationSeconds;
    b.timeRemaining = durationSeconds;
    b.bubbleColor = bubbleColor;
    b.textColor = textColor;
    m_bubbles.push_back(b);
}

void DialogController::showDialog(
    const std::string& speaker,
    const std::string& text,
    std::function<void()> onFinished
) {
    m_dialogActive = true;
    m_dialogSpeaker = speaker;
    m_dialogFullText = text;
    m_dialogRevealedChars = 0.0f;
    m_onDialogFinished = onFinished;
}

void DialogController::closeDialog() {
    if (m_dialogActive) {
        m_dialogActive = false;
        if (m_onDialogFinished) {
            auto cb = m_onDialogFinished;
            m_onDialogFinished = nullptr;
            cb();
        }
    }
}

bool DialogController::isDialogActive() const {
    return m_dialogActive;
}

void DialogController::nextDialog() {
    if (!m_dialogActive) return;

    if (m_dialogRevealedChars < static_cast<float>(m_dialogFullText.length())) {
        // Instant reveal
        m_dialogRevealedChars = static_cast<float>(m_dialogFullText.length());
    } else {
        closeDialog();
    }
}

void DialogController::setTypewriterSpeed(float charsPerSecond) {
    m_typewriterSpeed = std::max(5.0f, charsPerSecond);
}

void DialogController::showSubtitle(const std::string& text, float durationSeconds) {
    m_subtitleText = text;
    m_subtitleRemaining = durationSeconds;
}

void DialogController::update(float deltaTime) {
    // 1. Speech bubbles
    for (auto& b : m_bubbles) {
        b.timeRemaining -= deltaTime;
    }
    m_bubbles.erase(
        std::remove_if(m_bubbles.begin(), m_bubbles.end(),
            [](const SpeechBubble& b) { return b.timeRemaining <= 0.0f; }),
        m_bubbles.end()
    );

    // 2. Dialog typewriter
    if (m_dialogActive) {
        if (m_dialogRevealedChars < static_cast<float>(m_dialogFullText.length())) {
            m_dialogRevealedChars += m_typewriterSpeed * deltaTime;
        }
    }

    // 3. Subtitles
    if (m_subtitleRemaining > 0.0f) {
        m_subtitleRemaining -= deltaTime;
    }
}

void DialogController::render(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!renderer) return;

    // 1. Render Speech Bubbles
    for (const auto& b : m_bubbles) {
        Point sz = RenderController::instance().getTextSize(b.text, 0.9f);
        int pad = 12;
        int bw = sz.x + pad * 2;
        int bh = sz.y + pad * 2 + (b.speaker.empty() ? 0 : 20);

        int bx = b.x - bw / 2;
        int by = b.y - bh - 10;

        RenderController::instance().fillRect(renderer, Rect(bx, by, bw, bh), b.bubbleColor);
        RenderController::instance().drawRect(renderer, Rect(bx, by, bw, bh), Color(100, 150, 255, 200));

        int curY = by + pad;
        if (!b.speaker.empty()) {
            RenderController::instance().drawText(renderer, b.speaker, bx + pad, curY, Color::Yellow(), 0.85f);
            curY += 20;
        }
        RenderController::instance().drawText(renderer, b.text, bx + pad, curY, b.textColor, 0.9f);
    }

    // 2. Render RPG Dialog Box
    if (m_dialogActive) {
        int boxW = screenW - 140;
        int boxH = 130;
        int boxX = 70;
        int boxY = screenH - boxH - 30;

        // Background & glowing border
        RenderController::instance().fillRect(renderer, Rect(boxX, boxY, boxW, boxH), Color(12, 16, 26, 245));
        RenderController::instance().drawRect(renderer, Rect(boxX, boxY, boxW, boxH), Color::Cyan());
        RenderController::instance().drawRect(renderer, Rect(boxX + 2, boxY + 2, boxW - 4, boxH - 4), Color(30, 80, 130, 200));

        // Speaker Name Tag
        if (!m_dialogSpeaker.empty()) {
            Point nameSz = RenderController::instance().getTextSize(m_dialogSpeaker, 1.0f);
            RenderController::instance().fillRect(renderer, Rect(boxX + 16, boxY - 14, nameSz.x + 16, 26), Color(20, 30, 50, 250));
            RenderController::instance().drawRect(renderer, Rect(boxX + 16, boxY - 14, nameSz.x + 16, 26), Color::Yellow());
            RenderController::instance().drawText(renderer, m_dialogSpeaker, boxX + 24, boxY - 10, Color::Yellow(), 1.0f);
        }

        // Typewriter text
        size_t revCount = static_cast<size_t>(m_dialogRevealedChars);
        std::string currentText = m_dialogFullText.substr(0, std::min(revCount, m_dialogFullText.length()));
        RenderController::instance().drawText(renderer, currentText, boxX + 24, boxY + 28, Color::White(), 1.05f);

        // Continue indicator
        RenderController::instance().drawText(renderer, "[SPACE / Click to continue]", boxX + boxW - 220, boxY + boxH - 22, Color::Gray(180), 0.75f);
    }

    // 3. Render Cinematic Subtitle
    if (m_subtitleRemaining > 0.0f && !m_subtitleText.empty()) {
        Point sz = RenderController::instance().getTextSize(m_subtitleText, 1.0f);
        int sx = (screenW - sz.x) / 2;
        int sy = screenH - 65;

        RenderController::instance().fillRect(renderer, Rect(sx - 16, sy - 4, sz.x + 32, sz.y + 8), Color(0, 0, 0, 200));
        RenderController::instance().drawText(renderer, m_subtitleText, sx, sy, Color::White(), 1.0f);
    }
}

} // namespace sense

