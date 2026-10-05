#include "replay.hpp"
#include "player.hpp"
#include "game.hpp"
#include "render.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace sense {

ReplayController::ReplayController() = default;

ReplayController& ReplayController::instance() {
    static ReplayController inst;
    return inst;
}

void ReplayController::startRecording(const std::string& runName) {
    m_isRecording = true;
    m_recordTimer = 0.0f;
    m_currentRecording = ReplayRun();
    m_currentRecording.name = runName;
    RenderController::instance().showNotification("Replay: Recording Started!", 2.0f, Color::Green());
}

void ReplayController::stopRecording() {
    if (m_isRecording) {
        m_isRecording = false;
        m_currentRecording.totalTime = m_recordTimer;
        auto& pc = PlayerController::instance();
        if (pc.isValid()) {
            m_currentRecording.finalPosY = pc.getY();
        }
        RenderController::instance().showNotification("Replay: Recording Saved (" + std::to_string(m_currentRecording.samples.size()) + " frames)", 2.0f, Color::Cyan());
    }
}

bool ReplayController::isRecording() const {
    return m_isRecording;
}

const ReplayRun& ReplayController::getCurrentRun() const {
    return m_currentRecording;
}

void ReplayController::startPlayback(const ReplayRun& run) {
    if (!run.isValid()) return;
    m_playbackRun = run;
    m_playbackTimer = 0.0f;
    m_isPlayback = true;
    RenderController::instance().showNotification("Replay: Playback Active", 2.0f, Color::Yellow());
}

void ReplayController::stopPlayback() {
    m_isPlayback = false;
}

bool ReplayController::isPlaybackActive() const {
    return m_isPlayback;
}

float ReplayController::getPlaybackTime() const {
    return m_playbackTimer;
}

void ReplayController::setGhostEnabled(bool enabled) {
    m_ghostEnabled = enabled;
    m_liveGameTimer = 0.0f;
}

bool ReplayController::isGhostEnabled() const {
    return m_ghostEnabled;
}

void ReplayController::setGhostRun(const ReplayRun& run) {
    m_ghostRun = run;
}

const ReplayRun& ReplayController::getGhostRun() const {
    return m_ghostRun;
}

void ReplayController::setGhostColor(const Color& color) {
    m_ghostColor = color;
}

Color ReplayController::getGhostColor() const {
    return m_ghostColor;
}

void ReplayController::setGhostAlpha(uint8_t alpha) {
    m_ghostAlpha = alpha;
}

uint8_t ReplayController::getGhostAlpha() const {
    return m_ghostAlpha;
}

void ReplayController::update(float deltaTime) {
    auto& pc = PlayerController::instance();

    // 1. Recording loop
    if (m_isRecording && pc.isValid()) {
        if (pc.isMoving()) {
            m_recordTimer += deltaTime;

            static float s_sampleAccum = 0.0f;
            s_sampleAccum += deltaTime;
            if (s_sampleAccum >= 0.033f) { // ~30 Hz sampling
                s_sampleAccum = 0.0f;

                ReplaySample s;
                s.time = m_recordTimer;
                s.posY = pc.getY();
                s.speed = pc.getSpeed();
                s.currentFrame = pc.getCurrentFrame();
                s.move = pc.getLastMove();
                s.balance = pc.getBalance();
                m_currentRecording.samples.push_back(s);
            }
        }

        if (pc.hasLost() || pc.getY() >= 25000) {
            stopRecording();
        }
    }

    // 2. Ghost timer synchronization
    if (m_ghostEnabled && pc.isValid() && pc.isMoving()) {
        m_liveGameTimer += deltaTime;
    } else if (pc.isValid() && pc.getY() == 0 && !pc.isMoving()) {
        m_liveGameTimer = 0.0f;
    }

    // 3. Playback loop
    if (m_isPlayback && m_playbackRun.isValid() && pc.isValid()) {
        m_playbackTimer += deltaTime;
        if (m_playbackTimer > m_playbackRun.totalTime) {
            stopPlayback();
            return;
        }

        // Interpolate sample
        for (size_t i = 0; i < m_playbackRun.samples.size(); ++i) {
            if (m_playbackRun.samples[i].time >= m_playbackTimer) {
                const auto& s = m_playbackRun.samples[i];
                pc.teleport(s.posY);
                pc.setSpeed(s.speed);
                pc.setCurrentFrame(s.currentFrame);
                pc.setLastMove(s.move);
                break;
            }
        }
    }
}

void ReplayController::renderGhost(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_ghostEnabled || !m_ghostRun.isValid() || !renderer) return;

    auto& pc = PlayerController::instance();
    if (!pc.isValid()) return;

    // Find nearest sample to m_liveGameTimer
    const auto& samples = m_ghostRun.samples;
    auto it = std::lower_bound(samples.begin(), samples.end(), m_liveGameTimer,
        [](const ReplaySample& s, float t) { return s.time < t; });

    if (it == samples.end()) {
        it = samples.end() - 1;
    }

    int ghostY = it->posY;
    int playerY = pc.getY();
    int distDelta = ghostY - playerY;

    // Calculate ghost on-screen position relative to player
    const float baseWidth = 1280.0f;
    const float baseHeight = 720.0f;
    float scaleX = static_cast<float>(screenW) / baseWidth;
    float scaleY = static_cast<float>(screenH) / baseHeight;

    int ghostW = static_cast<int>(64 * scaleX);
    int ghostH = static_cast<int>(96 * scaleY);

    int playerScreenCenterX = screenW / 2;
    int playerScreenCenterY = static_cast<int>((screenH / 2) - (10 * scaleY));

    // World Y difference directly offsets vertical position
    int ghostScreenY = playerScreenCenterY - static_cast<int>(distDelta * scaleY);
    int ghostScreenX = playerScreenCenterX - (ghostW / 2);

    // Only draw if within reasonable screen range
    if (ghostScreenY > -100 && ghostScreenY < screenH + 100) {
        Color c = m_ghostColor;
        c.a = m_ghostAlpha;

        // Render translucent ghost silhouette / racer
        RenderController::instance().fillRect(renderer, Rect(ghostScreenX, ghostScreenY, ghostW, ghostH), c);
        RenderController::instance().drawRect(renderer, Rect(ghostScreenX, ghostScreenY, ghostW, ghostH), Color::White());

        // Ghost label
        std::string badge = std::string("GHOST: ") + (distDelta >= 0 ? "+" : "") + std::to_string(distDelta) + "m";
        RenderController::instance().drawText(renderer, badge, ghostScreenX - 10, ghostScreenY - 18, Color::Cyan(), 0.8f);
    }
}

bool ReplayController::saveReplayToFile(const ReplayRun& run, const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "[ReplayRun]\n";
    out << "name=" << run.name << "\n";
    out << "totalTime=" << run.totalTime << "\n";
    out << "finalPosY=" << run.finalPosY << "\n";
    out << "count=" << run.samples.size() << "\n";
    out << "[Samples]\n";

    for (const auto& s : run.samples) {
        out << s.time << "," << s.posY << "," << s.speed << "," << s.currentFrame << ","
            << static_cast<int>(s.move) << "," << s.balance << "\n";
    }

    return true;
}

bool ReplayController::loadReplayFromFile(const std::string& filePath, ReplayRun& outRun) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    outRun = ReplayRun();
    std::string line;
    bool inSamples = false;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line == "[Samples]") {
            inSamples = true;
            continue;
        }
        if (line == "[ReplayRun]") {
            inSamples = false;
            continue;
        }

        if (inSamples) {
            std::stringstream ss(line);
            std::string item;
            ReplaySample s;
            if (std::getline(ss, item, ',')) s.time = std::stof(item);
            if (std::getline(ss, item, ',')) s.posY = std::stoi(item);
            if (std::getline(ss, item, ',')) s.speed = std::stof(item);
            if (std::getline(ss, item, ',')) s.currentFrame = std::stoi(item);
            if (std::getline(ss, item, ',')) s.move = static_cast<PlayerMove>(std::stoi(item));
            if (std::getline(ss, item, ',')) s.balance = std::stof(item);
            outRun.samples.push_back(s);
        } else {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                if (k == "name") outRun.name = v;
                else if (k == "totalTime") outRun.totalTime = std::stof(v);
                else if (k == "finalPosY") outRun.finalPosY = std::stoi(v);
            }
        }
    }

    return outRun.isValid();
}

} // namespace sense

