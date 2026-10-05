#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace sense {

// ============================================================================
// Virtual File System & Asset Controller
// ============================================================================
class AssetController {
public:
    static AssetController& instance();

    // 1. Virtual File System (VFS)
    void mount(const std::string& virtualPath, const std::string& realPath);
    void unmount(const std::string& virtualPath);
    std::string resolvePath(const std::string& virtualPath) const;
    bool fileExists(const std::string& path) const;

    // 2. Runtime Texture Management
    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filePath);
    bool registerTexture(const std::string& id, SDL_Texture* texture);
    SDL_Texture* getTexture(const std::string& id) const;
    bool hasTexture(const std::string& id) const;
    void unloadTexture(const std::string& id);
    void unloadAllTextures();

    // 3. Texture & Asset Overrides
    void registerTextureOverride(const std::string& originalAsset, const std::string& replacementPath);
    std::string getTextureOverride(const std::string& originalAsset) const;
    bool hasTextureOverride(const std::string& originalAsset) const;
    void clearTextureOverrides();

    // 4. Custom Player Skin System
    void setCustomPlayerSkin(SDL_Texture* skinTexture);
    void setCustomPlayerSkinFromFile(SDL_Renderer* renderer, const std::string& filePath);
    SDL_Texture* getCustomPlayerSkin() const;
    bool hasCustomPlayerSkin() const;
    void resetCustomPlayerSkin();

    // 5. Audio Asset Overrides
    void registerAudioOverride(const std::string& originalAsset, const std::string& replacementPath);
    std::string getAudioOverride(const std::string& originalAsset) const;
    bool hasAudioOverride(const std::string& originalAsset) const;
    void clearAudioOverrides();

private:
    AssetController() = default;

    std::map<std::string, std::string> m_mountPoints;
    std::map<std::string, SDL_Texture*> m_loadedTextures;
    std::map<std::string, std::string> m_textureOverrides;
    std::map<std::string, std::string> m_audioOverrides;
    SDL_Texture* m_customPlayerSkin = nullptr;
};

// Convenience namespace functions
namespace assets {
    inline void mount(const std::string& vPath, const std::string& rPath) { AssetController::instance().mount(vPath, rPath); }
    inline std::string resolvePath(const std::string& vPath) { return AssetController::instance().resolvePath(vPath); }
    inline bool fileExists(const std::string& path) { return AssetController::instance().fileExists(path); }

    inline SDL_Texture* loadTexture(SDL_Renderer* r, const std::string& path) { return AssetController::instance().loadTexture(r, path); }
    inline bool registerTexture(const std::string& id, SDL_Texture* tex) { return AssetController::instance().registerTexture(id, tex); }
    inline SDL_Texture* getTexture(const std::string& id) { return AssetController::instance().getTexture(id); }
    inline bool hasTexture(const std::string& id) { return AssetController::instance().hasTexture(id); }
    inline void unloadTexture(const std::string& id) { AssetController::instance().unloadTexture(id); }

    inline void registerTextureOverride(const std::string& orig, const std::string& rep) { AssetController::instance().registerTextureOverride(orig, rep); }
    inline std::string getTextureOverride(const std::string& orig) { return AssetController::instance().getTextureOverride(orig); }
    inline bool hasTextureOverride(const std::string& orig) { return AssetController::instance().hasTextureOverride(orig); }

    inline void setCustomPlayerSkin(SDL_Texture* tex) { AssetController::instance().setCustomPlayerSkin(tex); }
    inline void setCustomPlayerSkinFromFile(SDL_Renderer* r, const std::string& path) { AssetController::instance().setCustomPlayerSkinFromFile(r, path); }
    inline SDL_Texture* getCustomPlayerSkin() { return AssetController::instance().getCustomPlayerSkin(); }
    inline bool hasCustomPlayerSkin() { return AssetController::instance().hasCustomPlayerSkin(); }
    inline void resetCustomPlayerSkin() { AssetController::instance().resetCustomPlayerSkin(); }

    inline void registerAudioOverride(const std::string& orig, const std::string& rep) { AssetController::instance().registerAudioOverride(orig, rep); }
    inline std::string getAudioOverride(const std::string& orig) { return AssetController::instance().getAudioOverride(orig); }
    inline bool hasAudioOverride(const std::string& orig) { return AssetController::instance().hasAudioOverride(orig); }
}

} // namespace sense

