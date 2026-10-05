#pragma once

#include "types.hpp"
#include <vector>
#include <functional>

namespace sense {

class EventBus {
public:
    static EventBus& instance();

    // Event Registration
    void subscribeInit(std::function<void()> handler);
    void subscribeUpdate(std::function<void(float)> handler);
    void subscribePreRender(std::function<void(SDL_Renderer*)> handler);
    void subscribeRender(std::function<void(SDL_Renderer*)> handler);
    void subscribeEvent(std::function<void(const SDL_Event&, bool&)> handler);
    void subscribeCheckpoint(std::function<void(CheckPoint)> handler);
    void subscribePlayerLost(std::function<void()> handler);
    void subscribePlayerWin(std::function<void()> handler);

    // Event Dispatching
    void dispatchInit();
    void dispatchUpdate(float deltaTime);
    void dispatchPreRender(SDL_Renderer* renderer);
    void dispatchRender(SDL_Renderer* renderer);
    bool dispatchEvent(const SDL_Event& event);
    void dispatchCheckpoint(CheckPoint cp);
    void dispatchPlayerLost();
    void dispatchPlayerWin();

private:
    EventBus() = default;

    std::vector<std::function<void()>> m_initHandlers;
    std::vector<std::function<void(float)>> m_updateHandlers;
    std::vector<std::function<void(SDL_Renderer*)>> m_preRenderHandlers;
    std::vector<std::function<void(SDL_Renderer*)>> m_renderHandlers;
    std::vector<std::function<void(const SDL_Event&, bool&)>> m_eventHandlers;
    std::vector<std::function<void(CheckPoint)>> m_checkpointHandlers;
    std::vector<std::function<void()>> m_lostHandlers;
    std::vector<std::function<void()>> m_winHandlers;
};

// Convenience namespace functions
namespace events {
    inline void onInit(std::function<void()> cb) { EventBus::instance().subscribeInit(cb); }
    inline void onUpdate(std::function<void(float)> cb) { EventBus::instance().subscribeUpdate(cb); }
    inline void onPreRender(std::function<void(SDL_Renderer*)> cb) { EventBus::instance().subscribePreRender(cb); }
    inline void onRender(std::function<void(SDL_Renderer*)> cb) { EventBus::instance().subscribeRender(cb); }
    inline void onEvent(std::function<void(const SDL_Event&, bool&)> cb) { EventBus::instance().subscribeEvent(cb); }
    inline void onCheckpoint(std::function<void(CheckPoint)> cb) { EventBus::instance().subscribeCheckpoint(cb); }
    inline void onPlayerLost(std::function<void()> cb) { EventBus::instance().subscribePlayerLost(cb); }
    inline void onPlayerWin(std::function<void()> cb) { EventBus::instance().subscribePlayerWin(cb); }
}

} // namespace sense

