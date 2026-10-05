#include "assets.hpp"
#include "lowlevel.hpp"
#include <windows.h>
#include <fstream>
#include <iostream>

namespace sense {

// Function pointer signatures from SDL2 and SDL2_image
typedef SDL_Texture* (*PFN_IMG_LoadTexture)(SDL_Renderer*, const char*);
typedef void*        (*PFN_IMG_Load)(const char*);
typedef SDL_Texture* (*PFN_SDL_CreateTextureFromSurface)(SDL_Renderer*, void*);
typedef void         (*PFN_SDL_FreeSurface)(void*);
typedef void         (*PFN_SDL_DestroyTexture)(SDL_Texture*);

static PFN_IMG_LoadTexture            g_fn_IMG_LoadTexture = nullptr;
static PFN_IMG_Load                   g_fn_IMG_Load = nullptr;
static PFN_SDL_CreateTextureFromSurface g_fn_SDL_CreateTextureFromSurface = nullptr;
static PFN_SDL_FreeSurface            g_fn_SDL_FreeSurface = nullptr;
static PFN_SDL_DestroyTexture         g_fn_SDL_DestroyTexture = nullptr;

static void initImageFunctions() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    HMODULE hImage = GetModuleHandleA("SDL2_image.dll");
    if (!hImage) {
        hImage = LoadLibraryA("SDL2_image.dll");
    }
    if (hImage) {
        g_fn_IMG_LoadTexture = reinterpret_cast<PFN_IMG_LoadTexture>(GetProcAddress(hImage, "IMG_LoadTexture"));
        g_fn_IMG_Load = reinterpret_cast<PFN_IMG_Load>(GetProcAddress(hImage, "IMG_Load"));
    }

    HMODULE hSdl = GetModuleHandleA("SDL2.dll");
    if (hSdl) {
        g_fn_SDL_CreateTextureFromSurface = reinterpret_cast<PFN_SDL_CreateTextureFromSurface>(GetProcAddress(hSdl, "SDL_CreateTextureFromSurface"));
        g_fn_SDL_FreeSurface = reinterpret_cast<PFN_SDL_FreeSurface>(GetProcAddress(hSdl, "SDL_FreeSurface"));
        g_fn_SDL_DestroyTexture = reinterpret_cast<PFN_SDL_DestroyTexture>(GetProcAddress(hSdl, "SDL_DestroyTexture"));
    }
}

AssetController& AssetController::instance() {
    static AssetController inst;
    return inst;
}

void AssetController::mount(const std::string& virtualPath, const std::string& realPath) {
    m_mountPoints[virtualPath] = realPath;
}

void AssetController::unmount(const std::string& virtualPath) {
    m_mountPoints.erase(virtualPath);
}

std::string AssetController::resolvePath(const std::string& virtualPath) const {
    for (const auto& [vPrefix, rPrefix] : m_mountPoints) {
        if (virtualPath.rfind(vPrefix, 0) == 0) {
            std::string sub = virtualPath.substr(vPrefix.length());
            if (!sub.empty() && (sub[0] == '/' || sub[0] == '\\')) {
                sub = sub.substr(1);
            }
            return rPrefix + "/" + sub;
        }
    }
    return virtualPath;
}

bool AssetController::fileExists(const std::string& path) const {
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

SDL_Texture* AssetController::loadTexture(SDL_Renderer* renderer, const std::string& filePath) {
    initImageFunctions();
    if (!renderer) return nullptr;

    std::string resolved = resolvePath(filePath);

    // Strategy 1: IMG_LoadTexture
    if (g_fn_IMG_LoadTexture) {
        SDL_Texture* tex = g_fn_IMG_LoadTexture(renderer, resolved.c_str());
        if (tex) return tex;
    }

    // Strategy 2: IMG_Load + SDL_CreateTextureFromSurface
    if (g_fn_IMG_Load && g_fn_SDL_CreateTextureFromSurface && g_fn_SDL_FreeSurface) {
        void* surface = g_fn_IMG_Load(resolved.c_str());
        if (surface) {
            SDL_Texture* tex = g_fn_SDL_CreateTextureFromSurface(renderer, surface);
            g_fn_SDL_FreeSurface(surface);
            if (tex) return tex;
        }
    }

    return nullptr;
}

bool AssetController::registerTexture(const std::string& id, SDL_Texture* texture) {
    if (!texture) return false;
    m_loadedTextures[id] = texture;
    return true;
}

SDL_Texture* AssetController::getTexture(const std::string& id) const {
    auto it = m_loadedTextures.find(id);
    return (it != m_loadedTextures.end()) ? it->second : nullptr;
}

bool AssetController::hasTexture(const std::string& id) const {
    return m_loadedTextures.find(id) != m_loadedTextures.end();
}

void AssetController::unloadTexture(const std::string& id) {
    initImageFunctions();
    auto it = m_loadedTextures.find(id);
    if (it != m_loadedTextures.end()) {
        if (g_fn_SDL_DestroyTexture && it->second) {
            g_fn_SDL_DestroyTexture(it->second);
        }
        m_loadedTextures.erase(it);
    }
}

void AssetController::unloadAllTextures() {
    initImageFunctions();
    for (auto& [id, tex] : m_loadedTextures) {
        if (g_fn_SDL_DestroyTexture && tex) {
            g_fn_SDL_DestroyTexture(tex);
        }
    }
    m_loadedTextures.clear();
}

void AssetController::registerTextureOverride(const std::string& originalAsset, const std::string& replacementPath) {
    m_textureOverrides[originalAsset] = replacementPath;
}

std::string AssetController::getTextureOverride(const std::string& originalAsset) const {
    auto it = m_textureOverrides.find(originalAsset);
    return (it != m_textureOverrides.end()) ? it->second : "";
}

bool AssetController::hasTextureOverride(const std::string& originalAsset) const {
    return m_textureOverrides.find(originalAsset) != m_textureOverrides.end();
}

void AssetController::clearTextureOverrides() {
    m_textureOverrides.clear();
}

void AssetController::setCustomPlayerSkin(SDL_Texture* skinTexture) {
    m_customPlayerSkin = skinTexture;
}

void AssetController::setCustomPlayerSkinFromFile(SDL_Renderer* renderer, const std::string& filePath) {
    SDL_Texture* tex = loadTexture(renderer, filePath);
    if (tex) {
        setCustomPlayerSkin(tex);
    }
}

SDL_Texture* AssetController::getCustomPlayerSkin() const {
    return m_customPlayerSkin;
}

bool AssetController::hasCustomPlayerSkin() const {
    return m_customPlayerSkin != nullptr;
}

void AssetController::resetCustomPlayerSkin() {
    m_customPlayerSkin = nullptr;
}

void AssetController::registerAudioOverride(const std::string& originalAsset, const std::string& replacementPath) {
    m_audioOverrides[originalAsset] = replacementPath;
}

std::string AssetController::getAudioOverride(const std::string& originalAsset) const {
    auto it = m_audioOverrides.find(originalAsset);
    return (it != m_audioOverrides.end()) ? it->second : "";
}

bool AssetController::hasAudioOverride(const std::string& originalAsset) const {
    return m_audioOverrides.find(originalAsset) != m_audioOverrides.end();
}

void AssetController::clearAudioOverrides() {
    m_audioOverrides.clear();
}

} // namespace sense

