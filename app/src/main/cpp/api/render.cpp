#include "render.hpp"
#include "profiler.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <map>
#include <cstring>

namespace sense {

// Function pointers to SDL2 runtime exports
typedef int (*PFN_SDL_SetRenderDrawBlendMode)(SDL_Renderer*, int);
typedef int (*PFN_SDL_SetRenderDrawColor)(SDL_Renderer*, uint8_t, uint8_t, uint8_t, uint8_t);
typedef int (*PFN_SDL_RenderFillRect)(SDL_Renderer*, const SDL_Rect*);
typedef int (*PFN_SDL_RenderDrawRect)(SDL_Renderer*, const SDL_Rect*);
typedef int (*PFN_SDL_RenderDrawLine)(SDL_Renderer*, int, int, int, int);
typedef int (*PFN_SDL_RenderDrawPoint)(SDL_Renderer*, int, int);
typedef int (*PFN_SDL_RenderGeometry)(SDL_Renderer*, SDL_Texture*, const SDL_Vertex*, int, const int*, int);
typedef void* (*PFN_SDL_RWFromConstMem)(const void*, int);
typedef SDL_Texture* (*PFN_SDL_CreateTextureFromSurface)(SDL_Renderer*, void*);
typedef void (*PFN_SDL_FreeSurface)(void*);
typedef int (*PFN_SDL_RenderCopy)(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*);
typedef void (*PFN_SDL_DestroyTexture)(SDL_Texture*);
typedef int (*PFN_SDL_QueryTexture)(SDL_Texture*, uint32_t*, int*, int*, int*);

static PFN_SDL_SetRenderDrawBlendMode fn_SDL_SetRenderDrawBlendMode = nullptr;
static PFN_SDL_SetRenderDrawColor fn_SDL_SetRenderDrawColor = nullptr;
static PFN_SDL_RenderFillRect fn_SDL_RenderFillRect = nullptr;
static PFN_SDL_RenderDrawRect fn_SDL_RenderDrawRect = nullptr;
static PFN_SDL_RenderDrawLine fn_SDL_RenderDrawLine = nullptr;
static PFN_SDL_RenderDrawPoint fn_SDL_RenderDrawPoint = nullptr;
static PFN_SDL_RenderGeometry fn_SDL_RenderGeometry = nullptr;
static PFN_SDL_RWFromConstMem fn_SDL_RWFromConstMem = nullptr;
static PFN_SDL_CreateTextureFromSurface fn_SDL_CreateTextureFromSurface = nullptr;
static PFN_SDL_FreeSurface fn_SDL_FreeSurface = nullptr;
static PFN_SDL_RenderCopy fn_SDL_RenderCopy = nullptr;
static PFN_SDL_DestroyTexture fn_SDL_DestroyTexture = nullptr;
static PFN_SDL_QueryTexture fn_SDL_QueryTexture = nullptr;

static void initRenderSdl() {
    static bool init = false;
    if (init) return;
    init = true;

    HMODULE hSdl = GetModuleHandleA("SDL2.dll");
    if (!hSdl) return;

    fn_SDL_SetRenderDrawBlendMode = reinterpret_cast<PFN_SDL_SetRenderDrawBlendMode>(GetProcAddress(hSdl, "SDL_SetRenderDrawBlendMode"));
    fn_SDL_SetRenderDrawColor = reinterpret_cast<PFN_SDL_SetRenderDrawColor>(GetProcAddress(hSdl, "SDL_SetRenderDrawColor"));
    fn_SDL_RenderFillRect = reinterpret_cast<PFN_SDL_RenderFillRect>(GetProcAddress(hSdl, "SDL_RenderFillRect"));
    fn_SDL_RenderDrawRect = reinterpret_cast<PFN_SDL_RenderDrawRect>(GetProcAddress(hSdl, "SDL_RenderDrawRect"));
    fn_SDL_RenderDrawLine = reinterpret_cast<PFN_SDL_RenderDrawLine>(GetProcAddress(hSdl, "SDL_RenderDrawLine"));
    fn_SDL_RenderDrawPoint = reinterpret_cast<PFN_SDL_RenderDrawPoint>(GetProcAddress(hSdl, "SDL_RenderDrawPoint"));
    fn_SDL_RenderGeometry = reinterpret_cast<PFN_SDL_RenderGeometry>(GetProcAddress(hSdl, "SDL_RenderGeometry"));
    fn_SDL_RWFromConstMem = reinterpret_cast<PFN_SDL_RWFromConstMem>(GetProcAddress(hSdl, "SDL_RWFromConstMem"));
    fn_SDL_CreateTextureFromSurface = reinterpret_cast<PFN_SDL_CreateTextureFromSurface>(GetProcAddress(hSdl, "SDL_CreateTextureFromSurface"));
    fn_SDL_FreeSurface = reinterpret_cast<PFN_SDL_FreeSurface>(GetProcAddress(hSdl, "SDL_FreeSurface"));
    fn_SDL_RenderCopy = reinterpret_cast<PFN_SDL_RenderCopy>(GetProcAddress(hSdl, "SDL_RenderCopy"));
    fn_SDL_DestroyTexture = reinterpret_cast<PFN_SDL_DestroyTexture>(GetProcAddress(hSdl, "SDL_DestroyTexture"));
    fn_SDL_QueryTexture = reinterpret_cast<PFN_SDL_QueryTexture>(GetProcAddress(hSdl, "SDL_QueryTexture"));
}

// Function pointers to SDL2_ttf runtime exports (already loaded in game process)
typedef void* TTF_Font;
typedef int (*PFN_TTF_Init)();
typedef int (*PFN_TTF_WasInit)();
typedef TTF_Font (*PFN_TTF_OpenFont)(const char*, int);
typedef TTF_Font (*PFN_TTF_OpenFontRW)(void*, int, int);
typedef void* (*PFN_TTF_RenderUTF8_Blended)(TTF_Font, const char*, SDL_Color);
typedef int (*PFN_TTF_SizeUTF8)(TTF_Font, const char*, int*, int*);
typedef void (*PFN_TTF_CloseFont)(TTF_Font);
typedef void (*PFN_TTF_SetFontHinting)(TTF_Font, int);
typedef int (*PFN_TTF_FontHeight)(TTF_Font);

static PFN_TTF_Init fn_TTF_Init = nullptr;
static PFN_TTF_WasInit fn_TTF_WasInit = nullptr;
static PFN_TTF_OpenFont fn_TTF_OpenFont = nullptr;
static PFN_TTF_OpenFontRW fn_TTF_OpenFontRW = nullptr;
static PFN_TTF_RenderUTF8_Blended fn_TTF_RenderUTF8_Blended = nullptr;
static PFN_TTF_SizeUTF8 fn_TTF_SizeUTF8 = nullptr;
static PFN_TTF_CloseFont fn_TTF_CloseFont = nullptr;
static PFN_TTF_SetFontHinting fn_TTF_SetFontHinting = nullptr;
static PFN_TTF_FontHeight fn_TTF_FontHeight = nullptr;

static void initTtf() {
    static bool init = false;
    if (init) return;
    init = true;

    HMODULE hTtf = GetModuleHandleA("SDL2_ttf.dll");
    if (!hTtf) hTtf = LoadLibraryA("SDL2_ttf.dll");
    if (!hTtf) return;

    fn_TTF_Init = reinterpret_cast<PFN_TTF_Init>(GetProcAddress(hTtf, "TTF_Init"));
    fn_TTF_WasInit = reinterpret_cast<PFN_TTF_WasInit>(GetProcAddress(hTtf, "TTF_WasInit"));
    fn_TTF_OpenFont = reinterpret_cast<PFN_TTF_OpenFont>(GetProcAddress(hTtf, "TTF_OpenFont"));
    fn_TTF_OpenFontRW = reinterpret_cast<PFN_TTF_OpenFontRW>(GetProcAddress(hTtf, "TTF_OpenFontRW"));
    fn_TTF_RenderUTF8_Blended = reinterpret_cast<PFN_TTF_RenderUTF8_Blended>(GetProcAddress(hTtf, "TTF_RenderUTF8_Blended"));
    fn_TTF_SizeUTF8 = reinterpret_cast<PFN_TTF_SizeUTF8>(GetProcAddress(hTtf, "TTF_SizeUTF8"));
    fn_TTF_CloseFont = reinterpret_cast<PFN_TTF_CloseFont>(GetProcAddress(hTtf, "TTF_CloseFont"));
    fn_TTF_SetFontHinting = reinterpret_cast<PFN_TTF_SetFontHinting>(GetProcAddress(hTtf, "TTF_SetFontHinting"));
    fn_TTF_FontHeight = reinterpret_cast<PFN_TTF_FontHeight>(GetProcAddress(hTtf, "TTF_FontHeight"));

    if (fn_TTF_WasInit && !fn_TTF_WasInit()) {
        if (fn_TTF_Init) fn_TTF_Init();
    }
}

// Find game's native TrueType font in memory (.rdata section) or on disk
static const uint8_t* findGameFontData(size_t& outSize) {
    outSize = 0;
    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    if (!base) return nullptr;

    // 1. Direct offset in SENSE_THE_GAME.exe (0x3EF90, 22736 bytes)
    const uint8_t* candidate = reinterpret_cast<const uint8_t*>(base + 0x3EF90);
    if (candidate[0] == 0x00 && candidate[1] == 0x01 && candidate[2] == 0x00 && candidate[3] == 0x00) {
        outSize = 22736;
        return candidate;
    }

    // 2. Dynamic scan in .rdata for TTF font signature (00 01 00 00 00 0C 00 80)
    const uint8_t* pBase = reinterpret_cast<const uint8_t*>(base);
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
    if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
        const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBase + dos->e_lfanew);
        if (nt->Signature == IMAGE_NT_SIGNATURE) {
            const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
                if (strncmp(reinterpret_cast<const char*>(sec[i].Name), ".rdata", 6) == 0) {
                    const uint8_t* rdata = pBase + sec[i].VirtualAddress;
                    DWORD size = sec[i].Misc.VirtualSize;
                    for (DWORD j = 0; j + 8 < size; j++) {
                        if (rdata[j] == 0x00 && rdata[j+1] == 0x01 && rdata[j+2] == 0x00 && rdata[j+3] == 0x00 &&
                            rdata[j+4] == 0x00 && rdata[j+5] == 0x0C && rdata[j+6] == 0x00 && rdata[j+7] == 0x80) {
                            outSize = 22736;
                            return &rdata[j];
                        }
                    }
                    break;
                }
            }
        }
    }

    return nullptr;
}

static std::map<int, TTF_Font> g_fontCache;

static TTF_Font getFont(int ptSize) {
    if (ptSize < 8) ptSize = 8;
    auto it = g_fontCache.find(ptSize);
    if (it != g_fontCache.end()) {
        return it->second;
    }

    initTtf();
    initRenderSdl();
    if (!fn_TTF_OpenFont && !fn_TTF_OpenFontRW) return nullptr;

    TTF_Font font = nullptr;

    // 1. Try font file on disk
    const char* fontPaths[] = {
        "assets/font/font.ttf",
        "font/font.ttf",
        "font.ttf",
        "../assets/font/font.ttf",
        "app/src/main/assets/font/font.ttf"
    };
    for (const char* path : fontPaths) {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            if (fn_TTF_OpenFont) {
                font = fn_TTF_OpenFont(path, ptSize);
                if (font) break;
            }
        }
    }

    // 2. Try embedded game font in memory
    if (!font && fn_TTF_OpenFontRW && fn_SDL_RWFromConstMem) {
        size_t fontDataSize = 0;
        const uint8_t* fontData = findGameFontData(fontDataSize);
        if (fontData && fontDataSize > 0) {
            void* rw = fn_SDL_RWFromConstMem(fontData, static_cast<int>(fontDataSize));
            if (rw) {
                font = fn_TTF_OpenFontRW(rw, 1 /* freesrc */, ptSize);
            }
        }
    }

    if (font) {
        if (fn_TTF_SetFontHinting) {
            fn_TTF_SetFontHinting(font, 1 /* TTF_HINTING_LIGHT */);
        }
        g_fontCache[ptSize] = font;
    }

    return font;
}

RenderController& RenderController::instance() {
    static RenderController inst;
    return inst;
}

void RenderController::drawRect(SDL_Renderer* renderer, const Rect& rect, const Color& color) {
    initRenderSdl();
    if (!renderer || !fn_SDL_SetRenderDrawColor || !fn_SDL_RenderDrawRect) return;

    ProfilerController::instance().recordDrawCall();
    fn_SDL_SetRenderDrawBlendMode(renderer, 1);
    fn_SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r = { rect.x, rect.y, rect.w, rect.h };
    fn_SDL_RenderDrawRect(renderer, &r);
}

void RenderController::fillRect(SDL_Renderer* renderer, const Rect& rect, const Color& color) {
    initRenderSdl();
    if (!renderer || !fn_SDL_SetRenderDrawColor || !fn_SDL_RenderFillRect) return;

    ProfilerController::instance().recordDrawCall();
    fn_SDL_SetRenderDrawBlendMode(renderer, 1);
    fn_SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r = { rect.x, rect.y, rect.w, rect.h };
    fn_SDL_RenderFillRect(renderer, &r);
}

void RenderController::drawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, const Color& color) {
    initRenderSdl();
    if (!renderer || !fn_SDL_SetRenderDrawColor || !fn_SDL_RenderDrawLine) return;

    ProfilerController::instance().recordDrawCall();
    fn_SDL_SetRenderDrawBlendMode(renderer, 1);
    fn_SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    fn_SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void RenderController::drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, const Color& color) {
    initRenderSdl();
    if (!renderer || !fn_SDL_RenderDrawPoint) return;

    ProfilerController::instance().recordDrawCall();
    fn_SDL_SetRenderDrawBlendMode(renderer, 1);
    fn_SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        fn_SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
        fn_SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
        fn_SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
        fn_SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
        fn_SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
        fn_SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
        fn_SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
        fn_SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void RenderController::fillCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, const Color& color) {
    initRenderSdl();
    if (!renderer || !fn_SDL_RenderDrawLine) return;

    fn_SDL_SetRenderDrawBlendMode(renderer, 1);
    fn_SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int w = 0; w <= radius * 2; w++) {
        for (int h = 0; h <= radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                fn_SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

void RenderController::drawGradientV(SDL_Renderer* renderer, const Rect& rect, const Color& topColor, const Color& bottomColor) {
    initRenderSdl();
    if (!renderer || rect.h <= 0) return;

    for (int y = 0; y < rect.h; ++y) {
        float t = static_cast<float>(y) / static_cast<float>(rect.h);
        uint8_t r = static_cast<uint8_t>(topColor.r + (bottomColor.r - topColor.r) * t);
        uint8_t g = static_cast<uint8_t>(topColor.g + (bottomColor.g - topColor.g) * t);
        uint8_t b = static_cast<uint8_t>(topColor.b + (bottomColor.b - topColor.b) * t);
        uint8_t a = static_cast<uint8_t>(topColor.a + (bottomColor.a - topColor.a) * t);

        fn_SDL_SetRenderDrawBlendMode(renderer, 1);
        fn_SDL_SetRenderDrawColor(renderer, r, g, b, a);
        fn_SDL_RenderDrawLine(renderer, rect.x, rect.y + y, rect.x + rect.w, rect.y + y);
    }
}

void RenderController::drawGradientH(SDL_Renderer* renderer, const Rect& rect, const Color& leftColor, const Color& rightColor) {
    initRenderSdl();
    if (!renderer || rect.w <= 0) return;

    for (int x = 0; x < rect.w; ++x) {
        float t = static_cast<float>(x) / static_cast<float>(rect.w);
        uint8_t r = static_cast<uint8_t>(leftColor.r + (rightColor.r - leftColor.r) * t);
        uint8_t g = static_cast<uint8_t>(leftColor.g + (rightColor.g - leftColor.g) * t);
        uint8_t b = static_cast<uint8_t>(leftColor.b + (rightColor.b - leftColor.b) * t);
        uint8_t a = static_cast<uint8_t>(leftColor.a + (rightColor.a - leftColor.a) * t);

        fn_SDL_SetRenderDrawBlendMode(renderer, 1);
        fn_SDL_SetRenderDrawColor(renderer, r, g, b, a);
        fn_SDL_RenderDrawLine(renderer, rect.x + x, rect.y, rect.x + x, rect.y + rect.h);
    }
}

void RenderController::clearFontCache() {
    if (fn_TTF_CloseFont) {
        for (auto& pair : g_fontCache) {
            if (pair.second) {
                fn_TTF_CloseFont(pair.second);
            }
        }
    }
    g_fontCache.clear();
}

void RenderController::drawText(
    SDL_Renderer* renderer,
    const std::string& text,
    int x,
    int y,
    const Color& color,
    float scale,
    bool shadow
) {
    initRenderSdl();
    initTtf();
    if (!renderer || text.empty()) return;

    int ptSize = static_cast<int>(15.0f * (scale > 0.0f ? scale : 1.0f));
    TTF_Font font = getFont(ptSize);
    if (!font || !fn_TTF_RenderUTF8_Blended || !fn_SDL_CreateTextureFromSurface) {
        return;
    }

    ProfilerController::instance().recordDrawCall();
    fn_SDL_SetRenderDrawBlendMode(renderer, 1);

    int curY = y;
    int lineHeight = fn_TTF_FontHeight ? fn_TTF_FontHeight(font) : (ptSize + 4);

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            curY += lineHeight;
            continue;
        }

        auto renderLine = [&](const Color& c, int posX, int posY) {
            SDL_Color fg{ c.r, c.g, c.b, c.a };
            void* surf = fn_TTF_RenderUTF8_Blended(font, line.c_str(), fg);
            if (!surf) return;

            SDL_Texture* tex = fn_SDL_CreateTextureFromSurface(renderer, surf);
            if (tex) {
                int w = 0, h = 0;
                if (fn_SDL_QueryTexture) {
                    fn_SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                }
                SDL_Rect dstRect{ posX, posY, w, h };
                if (fn_SDL_RenderCopy) {
                    fn_SDL_RenderCopy(renderer, tex, nullptr, &dstRect);
                }
                if (fn_SDL_DestroyTexture) {
                    fn_SDL_DestroyTexture(tex);
                }
            }
            if (fn_SDL_FreeSurface) {
                fn_SDL_FreeSurface(surf);
            }
        };

        if (shadow) {
            renderLine(Color(0, 0, 0, 180), x + 1, curY + 1);
        }
        renderLine(color, x, curY);

        curY += lineHeight;
    }
}

Point RenderController::getTextSize(const std::string& text, float scale) const {
    if (text.empty()) return { 0, 0 };

    int ptSize = static_cast<int>(15.0f * (scale > 0.0f ? scale : 1.0f));
    TTF_Font font = getFont(ptSize);
    if (!font || !fn_TTF_SizeUTF8) {
        return { static_cast<int>(text.length() * 9 * scale), static_cast<int>(18 * scale) };
    }

    std::istringstream stream(text);
    std::string line;
    int maxW = 0;
    int totalH = 0;
    int lineHeight = fn_TTF_FontHeight ? fn_TTF_FontHeight(font) : (ptSize + 4);

    while (std::getline(stream, line)) {
        if (line.empty()) {
            totalH += lineHeight;
            continue;
        }
        int w = 0, h = 0;
        if (fn_TTF_SizeUTF8(font, line.c_str(), &w, &h) == 0) {
            if (w > maxW) maxW = w;
            totalH += h;
        } else {
            totalH += lineHeight;
        }
    }

    return { maxW, totalH > 0 ? totalH : lineHeight };
}

void RenderController::drawTrapezoid(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const PointF& topLeft,
    const PointF& topRight,
    const PointF& bottomRight,
    const PointF& bottomLeft,
    const Color& color
) {
    initRenderSdl();
    if (!renderer || !fn_SDL_RenderGeometry) return;

    ProfilerController::instance().recordDrawCall();
    SDL_Color c = { color.r, color.g, color.b, color.a };
    SDL_Vertex vertices[4] = {
        { topLeft, c, { 0.0f, 0.0f } },
        { topRight, c, { 1.0f, 0.0f } },
        { bottomRight, c, { 1.0f, 1.0f } },
        { bottomLeft, c, { 0.0f, 1.0f } }
    };

    int indices[6] = { 0, 1, 2, 0, 2, 3 };
    fn_SDL_RenderGeometry(renderer, texture, vertices, 4, indices, 6);
}

void RenderController::drawTriangle(
    SDL_Renderer* renderer,
    const PointF& p1,
    const PointF& p2,
    const PointF& p3,
    const Color& color
) {
    initRenderSdl();
    if (!renderer || !fn_SDL_RenderGeometry) return;

    ProfilerController::instance().recordDrawCall();
    SDL_Color c = { color.r, color.g, color.b, color.a };
    SDL_Vertex vertices[3] = {
        { p1, c, { 0.0f, 0.0f } },
        { p2, c, { 1.0f, 0.0f } },
        { p3, c, { 0.5f, 1.0f } }
    };
    int indices[3] = { 0, 1, 2 };
    fn_SDL_RenderGeometry(renderer, nullptr, vertices, 3, indices, 3);
}

void RenderController::showNotification(const std::string& text, float durationSeconds, const Color& color) {
    ToastNotification toast;
    toast.text = text;
    toast.durationSeconds = durationSeconds;
    toast.timeRemaining = durationSeconds;
    toast.color = color;
    m_toasts.push_back(toast);
}

void RenderController::updateNotifications(float deltaTime) {
    for (auto it = m_toasts.begin(); it != m_toasts.end(); ) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f) {
            it = m_toasts.erase(it);
        } else {
            ++it;
        }
    }
}

void RenderController::renderNotifications(SDL_Renderer* renderer, int screenW, int screenH) {
    if (m_toasts.empty() || !renderer) return;

    int toastY = 20;
    for (const auto& toast : m_toasts) {
        float alphaFactor = 1.0f;
        if (toast.timeRemaining < 0.5f) {
            alphaFactor = toast.timeRemaining / 0.5f;
        }
        uint8_t alpha = static_cast<uint8_t>(255 * alphaFactor);

        Point size = getTextSize(toast.text, 1.2f);
        int padX = 14;
        int padY = 8;
        int boxW = size.x + padX * 2;
        int boxH = size.y + padY * 2;
        int boxX = (screenW - boxW) / 2;

        Rect bgRect(boxX, toastY, boxW, boxH);
        fillRect(renderer, bgRect, Color(20, 25, 35, static_cast<uint8_t>(230 * alphaFactor)));
        drawRect(renderer, bgRect, Color(toast.color.r, toast.color.g, toast.color.b, alpha));

        Color txtCol = Color(toast.color.r, toast.color.g, toast.color.b, alpha);
        drawText(renderer, toast.text, boxX + padX, toastY + padY, txtCol, 1.2f, true);

        toastY += boxH + 8;
    }
}

} // namespace sense

