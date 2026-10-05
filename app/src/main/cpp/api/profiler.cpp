#include "profiler.hpp"
#include "render.hpp"
#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace sense {

ProfilerController& ProfilerController::instance() {
    static ProfilerController inst;
    return inst;
}

ProfilerController::ProfilerController() = default;

void ProfilerController::recordFrame(float deltaTime) {
    if (deltaTime > 0.00001f) {
        m_currentFPS = 1.0f / deltaTime;
    }

    m_drawCallsLastFrame = m_drawCallsCurrentFrame;
    m_drawCallsCurrentFrame = 0;

    m_frameTimes.push_back(deltaTime);
    if (m_frameTimes.size() > 120) {
        m_frameTimes.pop_front();
    }
}

void ProfilerController::recordDrawCall() {
    m_drawCallsCurrentFrame++;
}

void ProfilerController::resetDrawCalls() {
    m_drawCallsCurrentFrame = 0;
}

float ProfilerController::getAverageFPS() const {
    if (m_frameTimes.empty()) return 60.0f;
    float sum = std::accumulate(m_frameTimes.begin(), m_frameTimes.end(), 0.0f);
    float avgDelta = sum / static_cast<float>(m_frameTimes.size());
    return (avgDelta > 0.00001f) ? (1.0f / avgDelta) : 60.0f;
}

float ProfilerController::getOnePercentLowFPS() const {
    if (m_frameTimes.empty()) return 60.0f;
    std::vector<float> sorted(m_frameTimes.begin(), m_frameTimes.end());
    std::sort(sorted.begin(), sorted.end(), std::greater<float>()); // Largest frame times first

    size_t count = (std::max)(size_t(1), sorted.size() / 100);
    float sum = 0.0f;
    for (size_t i = 0; i < count; i++) {
        sum += sorted[i];
    }
    float avgWorstDelta = sum / static_cast<float>(count);
    return (avgWorstDelta > 0.00001f) ? (1.0f / avgWorstDelta) : 60.0f;
}

ProcessMemoryStats ProfilerController::queryMemoryStats() const {
    ProcessMemoryStats stats;
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        stats.workingSetBytes = pmc.WorkingSetSize;
        stats.peakWorkingSetBytes = pmc.PeakWorkingSetSize;
        stats.privateBytes = pmc.PagefileUsage;
    }
    return stats;
}

void ProfilerController::renderOverlay(SDL_Renderer* renderer, int screenW, int screenH) {
    if (!m_overlayVisible || !renderer) return;

    auto& rc = RenderController::instance();
    int panelW = 230;
    int panelH = 175;
    int panelX = 20;
    int panelY = 50;

    // Background panel
    rc.fillRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(15, 20, 30, 230));
    rc.drawRect(renderer, Rect(panelX, panelY, panelW, panelH), Color(0, 200, 100, 180));

    // Title
    rc.drawText(renderer, "PERFORMANCE PROFILER", panelX + 10, panelY + 8, Color(0, 255, 130), 1.0f);
    rc.drawLine(renderer, panelX + 8, panelY + 26, panelX + panelW - 8, panelY + 26, Color(50, 80, 70, 150));

    // Stats
    float curFps = getCurrentFPS();
    float avgFps = getAverageFPS();
    float lowFps = getOnePercentLowFPS();
    float frameMs = (curFps > 0.0f) ? (1000.0f / curFps) : 16.6f;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "FPS: %.1f (Avg: %.1f | 1%%: %.1f)", curFps, avgFps, lowFps);
    rc.drawText(renderer, buf, panelX + 10, panelY + 32, (curFps >= 55.0f) ? Color::Green() : Color::Yellow(), 0.85f);

    std::snprintf(buf, sizeof(buf), "Frame Time: %.2f ms", frameMs);
    rc.drawText(renderer, buf, panelX + 10, panelY + 48, Color(200, 210, 225), 0.85f);

    std::snprintf(buf, sizeof(buf), "Draw Calls: %d", m_drawCallsLastFrame);
    rc.drawText(renderer, buf, panelX + 10, panelY + 64, Color(200, 210, 225), 0.85f);

    ProcessMemoryStats mem = queryMemoryStats();
    double ramMb = static_cast<double>(mem.workingSetBytes) / (1024.0 * 1024.0);
    double peakMb = static_cast<double>(mem.peakWorkingSetBytes) / (1024.0 * 1024.0);
    std::snprintf(buf, sizeof(buf), "RAM: %.1f MB (Peak: %.1f MB)", ramMb, peakMb);
    rc.drawText(renderer, buf, panelX + 10, panelY + 80, Color(160, 200, 240), 0.85f);

    // Frame Time Graph (History)
    int graphX = panelX + 10;
    int graphY = panelY + 100;
    int graphW = panelW - 20;
    int graphH = 65;

    rc.fillRect(renderer, Rect(graphX, graphY, graphW, graphH), Color(8, 12, 18, 240));
    rc.drawRect(renderer, Rect(graphX, graphY, graphW, graphH), Color(40, 60, 50, 200));

    // Threshold lines: 16.6ms (60fps) and 33.3ms (30fps)
    // Scale: 0 to 50ms = graphH
    auto msToY = [&](float ms) {
        float norm = ms / 50.0f;
        if (norm > 1.0f) norm = 1.0f;
        return graphY + graphH - static_cast<int>(norm * graphH);
    };

    int y60 = msToY(16.66f);
    int y30 = msToY(33.33f);
    rc.drawLine(renderer, graphX, y60, graphX + graphW, y60, Color(0, 180, 80, 80));
    rc.drawLine(renderer, graphX, y30, graphX + graphW, y30, Color(180, 150, 0, 80));

    if (!m_frameTimes.empty()) {
        int count = static_cast<int>(m_frameTimes.size());
        int startIdx = (count > graphW) ? (count - graphW) : 0;
        int drawCount = count - startIdx;

        for (int i = 0; i < drawCount; i++) {
            float ms = m_frameTimes[startIdx + i] * 1000.0f;
            int barY = msToY(ms);
            int barX = graphX + graphW - drawCount + i;

            Color barCol = (ms <= 17.5f) ? Color(0, 220, 100) : (ms <= 34.0f ? Color(255, 200, 0) : Color(255, 60, 60));
            rc.drawLine(renderer, barX, graphY + graphH - 1, barX, barY, barCol);
        }
    }
}

} // namespace sense

