#include "gameobjects.hpp"
#include <algorithm>

namespace sense {

GameObjectsController::GameObjectsController() = default;

GameObjectsController& GameObjectsController::instance() {
    static GameObjectsController instance;
    return instance;
}

int GameObjectsController::addModifier(const GameObjectModifier& modifier) {
    GameObjectModifier m = modifier;
    m.id = m_nextModifierId++;
    m_modifiers.push_back(m);
    return m.id;
}

bool GameObjectsController::removeModifier(int modifierId) {
    auto it = std::remove_if(m_modifiers.begin(), m_modifiers.end(),
        [modifierId](const GameObjectModifier& m) { return m.id == modifierId; });
    if (it != m_modifiers.end()) {
        m_modifiers.erase(it, m_modifiers.end());
        return true;
    }
    return false;
}

void GameObjectsController::clearModifiers() {
    m_modifiers.clear();
}

GameObjectModifier* GameObjectsController::getModifier(int modifierId) {
    for (auto& m : m_modifiers) {
        if (m.id == modifierId) return &m;
    }
    return nullptr;
}

int GameObjectsController::setGlobalTint(const Color& tint) {
    GameObjectModifier mod;
    mod.name = "Global Tint";
    mod.tint = tint;
    return addModifier(mod);
}

int GameObjectsController::hideObjectByTexture(SDL_Texture* tex) {
    GameObjectModifier mod;
    mod.name = "Hide Texture";
    mod.targetTexture = tex;
    mod.hide = true;
    return addModifier(mod);
}

int GameObjectsController::replaceTexture(SDL_Texture* original, SDL_Texture* replacement) {
    GameObjectModifier mod;
    mod.name = "Replace Texture";
    mod.targetTexture = original;
    mod.replacementTexture = replacement;
    return addModifier(mod);
}

int GameObjectsController::scaleObjects(float sx, float sy) {
    GameObjectModifier mod;
    mod.name = "Scale Objects";
    mod.scaleX = sx;
    mod.scaleY = sy;
    return addModifier(mod);
}

int GameObjectsController::offsetObjects(int dx, int dy) {
    GameObjectModifier mod;
    mod.name = "Offset Objects";
    mod.offsetX = dx;
    mod.offsetY = dy;
    return addModifier(mod);
}

void GameObjectsController::onNewFrame() {
    // Clear temporary introspection state if needed, or maintain map
}

bool GameObjectsController::interceptRenderCopy(
    SDL_Renderer* renderer,
    SDL_Texture*& texture,
    const Rect*& srcrect,
    Rect& dstrect,
    double& angle,
    int& flip,
    Color& tint
) {
    if (!texture) return true;

    // Track captured object
    auto& cap = m_captured[texture];
    cap.texture = texture;
    if (srcrect) cap.srcRect = *srcrect;
    cap.dstRect = dstrect;
    cap.angle = angle;
    cap.flip = flip;
    cap.frameCount++;

    // Apply active modifiers
    for (auto& mod : m_modifiers) {
        if (!mod.enabled) continue;

        // Texture match filter
        if (mod.targetTexture && mod.targetTexture != texture) {
            continue;
        }

        // Dimension filter
        if (dstrect.w < mod.minWidth || dstrect.w > mod.maxWidth ||
            dstrect.h < mod.minHeight || dstrect.h > mod.maxHeight) {
            continue;
        }

        // Hide filter
        if (mod.hide) {
            return false;
        }

        // Texture replacement
        if (mod.replacementTexture) {
            texture = mod.replacementTexture;
        }

        // Tint modulation
        if (mod.tint.r != 255 || mod.tint.g != 255 || mod.tint.b != 255 || mod.tint.a != 255) {
            tint = mod.tint;
        }

        // Scale & Offset
        if (mod.scaleX != 1.0f || mod.scaleY != 1.0f) {
            int newW = static_cast<int>(dstrect.w * mod.scaleX);
            int newH = static_cast<int>(dstrect.h * mod.scaleY);
            dstrect.x += (dstrect.w - newW) / 2;
            dstrect.y += (dstrect.h - newH) / 2;
            dstrect.w = newW;
            dstrect.h = newH;
        }
        dstrect.x += mod.offsetX;
        dstrect.y += mod.offsetY;

        // Angle & Flip
        angle += mod.angleOffset;
        if (mod.flip != 0) {
            flip = mod.flip;
        }

        // Dynamic callback
        if (mod.onIntercept) {
            bool keep = mod.onIntercept(renderer, texture, srcrect, dstrect, angle, flip, tint);
            if (!keep) return false;
        }
    }

    return true;
}

} // namespace sense

