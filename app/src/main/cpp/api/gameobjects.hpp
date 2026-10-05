#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace sense {

// ============================================================================
// 1. Game Object Filter & Rule Specification
// ============================================================================
enum class GameObjectCategory : int {
    All = 0,
    Player,
    Track,
    Background,
    UI,
    Custom
};

struct GameObjectModifier {
    int id = 0;
    std::string name;
    bool enabled = true;

    // Matching criteria
    SDL_Texture* targetTexture = nullptr; // Match specific texture pointer (null = match any)
    Rect sourceRectFilter = { 0, 0, 0, 0 }; // Match source rect (w=0, h=0 matches any)
    int minWidth = 0;
    int maxWidth = 99999;
    int minHeight = 0;
    int maxHeight = 99999;

    // Modifications
    bool hide = false;                    // Suppress rendering completely
    SDL_Texture* replacementTexture = nullptr; // Replace texture with custom texture
    Color tint = Color::White();          // Color modulation / tinting
    float scaleX = 1.0f;                  // Scale factor X
    float scaleY = 1.0f;                  // Scale factor Y
    int offsetX = 0;                      // Translation X in pixels
    int offsetY = 0;                      // Translation Y in pixels
    double angleOffset = 0.0;             // Additional rotation angle
    int flip = 0;                         // SDL_RendererFlip override (0=none, 1=horizontal, 2=vertical)

    // Dynamic callback
    std::function<bool(SDL_Renderer* renderer, SDL_Texture*& tex, const Rect*& src, Rect& dst, double& angle, int& flip, Color& tint)> onIntercept;
};

// ============================================================================
// 2. Tracked Game Object Descriptor
// ============================================================================
struct CapturedGameObject {
    SDL_Texture* texture = nullptr;
    Rect srcRect;
    Rect dstRect;
    double angle = 0.0;
    int flip = 0;
    int frameCount = 0;
};

// ============================================================================
// 3. Master Game Objects Controller
// ============================================================================
class GameObjectsController {
public:
    static GameObjectsController& instance();

    // Modifier Registration
    int addModifier(const GameObjectModifier& modifier);
    bool removeModifier(int modifierId);
    void clearModifiers();
    GameObjectModifier* getModifier(int modifierId);
    const std::vector<GameObjectModifier>& getModifiers() const { return m_modifiers; }

    // Quick Modifications
    int setGlobalTint(const Color& tint);
    int hideObjectByTexture(SDL_Texture* tex);
    int replaceTexture(SDL_Texture* original, SDL_Texture* replacement);
    int scaleObjects(float sx, float sy);
    int offsetObjects(int dx, int dy);

    // Frame Lifecycle
    void onNewFrame();

    // Render Hook Interceptor (called by core.cpp in Hooked_SDL_RenderCopy / Ex)
    bool interceptRenderCopy(
        SDL_Renderer* renderer,
        SDL_Texture*& texture,
        const Rect*& srcrect,
        Rect& dstrect,
        double& angle,
        int& flip,
        Color& tint
    );

    // Introspection
    const std::map<SDL_Texture*, CapturedGameObject>& getCapturedObjects() const { return m_captured; }
    size_t getCapturedCount() const { return m_captured.size(); }

private:
    GameObjectsController();

    int m_nextModifierId = 1;
    std::vector<GameObjectModifier> m_modifiers;
    std::map<SDL_Texture*, CapturedGameObject> m_captured;
};

// ============================================================================
// Convenience namespace
// ============================================================================
namespace gameobjects {
    inline int addModifier(const GameObjectModifier& mod) { return GameObjectsController::instance().addModifier(mod); }
    inline bool removeModifier(int id) { return GameObjectsController::instance().removeModifier(id); }
    inline void clearModifiers() { GameObjectsController::instance().clearModifiers(); }
    inline int setGlobalTint(const Color& c) { return GameObjectsController::instance().setGlobalTint(c); }
    inline int replaceTexture(SDL_Texture* orig, SDL_Texture* rep) { return GameObjectsController::instance().replaceTexture(orig, rep); }
    inline int scaleObjects(float sx, float sy) { return GameObjectsController::instance().scaleObjects(sx, sy); }
    inline int offsetObjects(int dx, int dy) { return GameObjectsController::instance().offsetObjects(dx, dy); }
}

} // namespace sense

