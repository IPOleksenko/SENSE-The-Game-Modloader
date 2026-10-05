#include "speedrun.hpp"
#include "player.hpp"
#include "render.hpp"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace sense {

SpeedrunController& SpeedrunController::instance() {
    static SpeedrunController inst;
    return inst;
}

SpeedrunController::SpeedrunController() {
    // Default splits for SENSE: The Game
    addSplit("Sector 1 (5k)", 5000.0f, 25.0);
    addSplit("Sector 2 (10k)", 10000.0f, 52.0);
    addSplit("Sector 3 (15k)", 15000.0f, 80.0);
    addSplit("Sector 4 (20k)", 20000.0f, 110.0);
    addSplit("Finish (25k)", 25000.0f, 140.0);
}

void SpeedrunController::start() {
    m_running = true;
    m_paused = false;
    m_elapsedTime = 0.0;
    m_currentSplitIndex = 0;
    for (auto& s : m_splits) {
        s.completed = false;
        s.currentTime = 0.0;
    }
}

void SpeedrunController::pause() {
    m_paused = true;
}

void SpeedrunController::resume() {
    m_paused = false;
}

void SpeedrunController::reset() {
    m_running = false;
    m_paused = false;
    m_elapsedTime = 0.0;
    m_currentSplitIndex = 0;
    for (auto& s : m_splits) {
        s.completed = false;
        s.currentTime = 0.0;
    }
}

double SpeedrunController::getElapsedTime() const {
    return m_elapsedTime;
}

void SpeedrunController::addSplit(const std::string& name, float targetY, double pbTime) {
    SplitSegment seg;
    seg.name = name;
    seg.targetY = targetY;
    seg.pbTime = pbTime;
    seg.currentTime = 0.0;
    seg.completed = false;
    m_splits.push_back(seg);
}

void SpeedrunController::clearSplits() {
    m_splits.clear();
    m_currentSplitIndex = 0;
}

void SpeedrunController::savePBToFile(const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return;
    for (const auto& s : m_splits) {
        out << s.name << "," << s.targetY << "," << s.pbTime << "\n";
    }
}

void SpeedrunController::loadPBFromFile(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return;
    std::string line;
    size_t idx = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string name, yStr, pbStr;
        if (std::getline(ss, name, ',') && std::getline(ss, yStr, ',') && std::getline(ss, pbStr, ',')) {
            if (idx < m_splits.size()) {
                m_splits[idx].pbTime = std::stod(pbStr);
            }
        }
        idx++;
    }
}

std::string SpeedrunController::formatTime(double seconds) {
    int totalSec = static_cast<int>(seconds);
    int minutes = totalSec / 60;
    int secs = totalSec % 60;
    int hundredths = static_cast<int>((seconds - totalSec) * 100.0);
    if (hundredths < 0) hundredths = 0;
    if (hundredths > 99) hundredths = 99;

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d", minutes, secs, hundredths);
    return std::string(buf);
}

void SpeedrunController::update(float deltaTime) {
    auto& pc = PlayerController::instance();

    // Auto-start on movement from start line
    if (!m_running && pc.isValid() && pc.isMoving() && pc.getY() > 50.0f) {
        start();
    }

    if (m_running && !m_paused) {
        m_elapsedTime += static_cast<double>(deltaTime);

        if (pc.isValid()) {
            float playerY = static_cast<float>(pc.getY());
            while (m_currentSplitIndex < m_splits.size()) {
                auto& s = m_splits[m_currentSplitIndex];
                if (playerY >= s.targetY) {
                    s.completed = true;
                    s.currentTime = m_elapsedTime;
                    m_currentSplitIndex++;
                } else {
                    break;
                }
            }

            // Finish check
            if (m_currentSplitIndex >= m_splits.size()) {
                m_running = false; // Finished!
            }
        }
    }
}

void SpeedrunController::renderOverlay(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_overlayVisible || !renderer) return;

    auto& rc = RenderController::instance();
    int panelW = 240;
    int panelH = 36 + static_cast<int>(m_splits.size()) * 22 + 36;
    int panelX = screenW - panelW - 20;
    int panelY = 50;

    // Background & Border
    rc.fillRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(15, 18, 26, 220));
    rc.drawRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(0, 180, 255, 180));

    // Header
    rc.drawText(renderer, "LIVE SPLITS", panelX + 12, panelY + 8, Color(0, 220, 255), 1.0f);
    rc.drawLine(renderer, panelX + 8, panelY + 28, panelX + panelW - 8, panelY + 28, Color(60, 80, 120, 150));

    int curY = panelY + 34;
    for (size_t i = 0; i < m_splits.size(); i++) {
        const auto& s = m_splits[i];
        Color rowColor = Color(180, 190, 210);
        if (i == m_currentSplitIndex && m_running) {
            rowColor = Color(255, 255, 100); // Current split highlighting
        }

        rc.drawText(renderer, s.name, panelX + 12, curY, rowColor, 0.85f);

        std::string timeStr;
        Color deltaColor = Color::White();
        if (s.completed) {
            timeStr = formatTime(s.currentTime);
            if (s.pbTime > 0.0) {
                double diff = s.currentTime - s.pbTime;
                char diffBuf[32];
                if (diff < 0) {
                    std::snprintf(diffBuf, sizeof(diffBuf), "-%.2f", -diff);
                    deltaColor = Color::Green();
                } else {
                    std::snprintf(diffBuf, sizeof(diffBuf), "+%.2f", diff);
                    deltaColor = Color::Red();
                }
                timeStr += " (" + std::string(diffBuf) + ")";
            }
        } else {
            timeStr = (s.pbTime > 0.0) ? formatTime(s.pbTime) : "-:--.--";
            deltaColor = Color(120, 130, 150);
        }

        rc.drawText(renderer, timeStr, panelX + panelW - 110, curY, deltaColor, 0.85f);
        curY += 22;
    }

    // Footer divider and elapsed timer
    rc.drawLine(renderer, panelX + 8, curY + 2, panelX + panelW - 8, curY + 2, Color(60, 80, 120, 150));
    std::string totalStr = formatTime(m_elapsedTime);
    rc.drawText(renderer, totalStr, panelX + panelW - 120, curY + 8, Color(255, 255, 255), 1.1f);
}

} // namespace sense

