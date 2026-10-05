#include "lowlevel.hpp"
#include <sstream>
#include <iostream>
#include <psapi.h>

#pragma comment(lib, "user32.lib")

namespace sense {
namespace lowlevel {

static HWND g_gameHwnd = nullptr;
static WNDPROC g_originalWndProc = nullptr;
static void* g_rawPlayer = nullptr;
static SDL_Renderer* g_rawRenderer = nullptr;
static SDL_Window* g_rawWindow = nullptr;

// ============================================================================
// 1. x64 Inline Detour Hook Engine
// ============================================================================
DetourHook::DetourHook() = default;

DetourHook::~DetourHook() {
    remove();
}

bool DetourHook::create(void* targetFunction, void* detourFunction, void** outOriginalTrampoline) {
    if (!targetFunction || !detourFunction) return false;

    // Prevent re-detouring if target already begins with 64-bit absolute jump (FF 25 00 00 00 00)
    const uint8_t* checkPtr = static_cast<const uint8_t*>(targetFunction);
    if (checkPtr[0] == 0xFF && checkPtr[1] == 0x25) {
        return false;
    }

    m_target = targetFunction;
    m_detour = detourFunction;
    m_patchSize = 14; // Standard 64-bit absolute jump: FF 25 00 00 00 00 [8-byte address]

    // Allocate executable memory for trampoline
    m_trampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!m_trampoline) return false;

    // Read and save original bytes
    m_originalBytes.resize(m_patchSize);
    memcpy(m_originalBytes.data(), m_target, m_patchSize);

    // Copy original instructions into trampoline
    memcpy(m_trampoline, m_target, m_patchSize);

    // Append jump back from trampoline to target + 14
    uint8_t* trampPtr = static_cast<uint8_t*>(m_trampoline) + m_patchSize;
    trampPtr[0] = 0xFF;
    trampPtr[1] = 0x25;
    *reinterpret_cast<int32_t*>(trampPtr + 2) = 0; // RIP-relative offset 0
    *reinterpret_cast<uintptr_t*>(trampPtr + 6) = reinterpret_cast<uintptr_t>(m_target) + m_patchSize;

    FlushInstructionCache(GetCurrentProcess(), m_trampoline, 64);

    if (outOriginalTrampoline) {
        *outOriginalTrampoline = m_trampoline;
    }

    return enable();
}

bool DetourHook::enable() {
    if (!m_target || !m_detour || m_enabled) return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(m_target, m_patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    // Write 64-bit absolute jump: JMP [RIP+0] -> FF 25 00 00 00 00 [detour]
    uint8_t* targetPtr = static_cast<uint8_t*>(m_target);
    targetPtr[0] = 0xFF;
    targetPtr[1] = 0x25;
    *reinterpret_cast<int32_t*>(targetPtr + 2) = 0;
    *reinterpret_cast<uintptr_t*>(targetPtr + 6) = reinterpret_cast<uintptr_t>(m_detour);

    VirtualProtect(m_target, m_patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), m_target, m_patchSize);

    m_enabled = true;
    return true;
}

bool DetourHook::disable() {
    if (!m_target || !m_enabled || m_originalBytes.empty()) return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(m_target, m_patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    memcpy(m_target, m_originalBytes.data(), m_patchSize);
    VirtualProtect(m_target, m_patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), m_target, m_patchSize);

    m_enabled = false;
    return true;
}

void DetourHook::remove() {
    disable();
    if (m_trampoline) {
        VirtualFree(m_trampoline, 0, MEM_RELEASE);
        m_trampoline = nullptr;
    }
    m_target = nullptr;
    m_detour = nullptr;
}

// ============================================================================
// 2. VMT Hooking
// ============================================================================
bool hookVirtualMethod(void* classInstance, size_t methodIndex, void* newMethod, void** outOriginalMethod) {
    if (!classInstance || !newMethod) return false;

    uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(classInstance);
    if (!vtable) return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(&vtable[methodIndex], sizeof(uintptr_t), PAGE_READWRITE, &oldProtect)) {
        return false;
    }

    if (outOriginalMethod) {
        *outOriginalMethod = reinterpret_cast<void*>(vtable[methodIndex]);
    }

    vtable[methodIndex] = reinterpret_cast<uintptr_t>(newMethod);
    VirtualProtect(&vtable[methodIndex], sizeof(uintptr_t), oldProtect, &oldProtect);
    return true;
}

// ============================================================================
// 3. Memory Reading, Writing & Patching
// ============================================================================
bool protect(uintptr_t address, size_t size, DWORD newProtect, DWORD* outOldProtect) {
    DWORD oldP = 0;
    BOOL res = VirtualProtect(reinterpret_cast<void*>(address), size, newProtect, &oldP);
    if (outOldProtect) *outOldProtect = oldP;
    return res != FALSE;
}

bool readBytes(uintptr_t address, void* buffer, size_t size) {
    if (!address || !buffer || size == 0) return false;
    __try {
        memcpy(buffer, reinterpret_cast<const void*>(address), size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool writeBytes(uintptr_t address, const void* buffer, size_t size) {
    if (!address || !buffer || size == 0) return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    bool success = false;
    __try {
        memcpy(reinterpret_cast<void*>(address), buffer, size);
        success = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        success = false;
    }

    VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(address), size);
    return success;
}

bool patch(uintptr_t address, const std::vector<uint8_t>& bytes) {
    return writeBytes(address, bytes.data(), bytes.size());
}

bool nop(uintptr_t address, size_t count) {
    std::vector<uint8_t> nops(count, 0x90);
    return patch(address, nops);
}

// ============================================================================
// 4. Pattern / Signature Scanner
// ============================================================================
uintptr_t getModuleBase(const char* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
}

size_t getModuleSize(const char* moduleName) {
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod) return 0;
    MODULEINFO modInfo = {};
    if (GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo))) {
        return modInfo.SizeOfImage;
    }
    return 0;
}

uintptr_t scanMemory(uintptr_t startAddress, size_t searchSize, const char* pattern, const char* mask) {
    if (!startAddress || !searchSize || !pattern || !mask) return 0;
    size_t patternLen = strlen(mask);

    for (size_t i = 0; i <= searchSize - patternLen; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] != '?' && pattern[j] != *reinterpret_cast<const char*>(startAddress + i + j)) {
                found = false;
                break;
            }
        }
        if (found) {
            return startAddress + i;
        }
    }
    return 0;
}

uintptr_t findPattern(const char* moduleName, const char* pattern, const char* mask) {
    uintptr_t base = getModuleBase(moduleName);
    size_t size = getModuleSize(moduleName);
    if (!base || !size) return 0;
    return scanMemory(base, size, pattern, mask);
}

uintptr_t findSignature(const char* moduleName, const std::string& idaSignature) {
    std::vector<char> pattern;
    std::string mask;
    std::istringstream stream(idaSignature);
    std::string byteStr;

    while (stream >> byteStr) {
        if (byteStr == "?" || byteStr == "??") {
            pattern.push_back('\0');
            mask.push_back('?');
        } else {
            pattern.push_back(static_cast<char>(strtoul(byteStr.c_str(), nullptr, 16)));
            mask.push_back('x');
        }
    }

    if (mask.empty()) return 0;
    return findPattern(moduleName, pattern.data(), mask.c_str());
}

// ============================================================================
// 5. Win32 Window Subclassing & Manipulation
// ============================================================================
static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) {
        char title[128] = {};
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strstr(title, "SENSE") != nullptr) {
            *reinterpret_cast<HWND*>(lParam) = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

HWND getWindowHandle() {
    if (g_gameHwnd && IsWindow(g_gameHwnd)) {
        return g_gameHwnd;
    }

    g_gameHwnd = FindWindowA(nullptr, "SENSE: The Game");
    if (!g_gameHwnd) {
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&g_gameHwnd));
    }
    return g_gameHwnd;
}

void setWindowHandle(HWND hwnd) {
    g_gameHwnd = hwnd;
}

bool hookWndProc(WndProcCallback newWndProc) {
    HWND hwnd = getWindowHandle();
    if (!hwnd || !newWndProc) return false;

    g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(newWndProc)));
    return g_originalWndProc != nullptr;
}

LRESULT callOriginalWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_originalWndProc) {
        return CallWindowProcA(g_originalWndProc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

void setWindowTransparency(uint8_t alpha) {
    HWND hwnd = getWindowHandle();
    if (!hwnd) return;

    LONG_PTR exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_LAYERED)) {
        SetWindowLongPtrA(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
    }
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

void setWindowTitle(const std::string& title) {
    HWND hwnd = getWindowHandle();
    if (hwnd) {
        SetWindowTextA(hwnd, title.c_str());
    }
}

void setWindowBorderless(bool borderless) {
    HWND hwnd = getWindowHandle();
    if (!hwnd) return;

    LONG_PTR style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    if (borderless) {
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    } else {
        style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    }
    SetWindowLongPtrA(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void setWindowPosition(int x, int y, int width, int height) {
    HWND hwnd = getWindowHandle();
    if (hwnd) {
        SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER);
    }
}

// ============================================================================
// 6. Live Raw Pointers
// ============================================================================
void* getRawPlayer() { return g_rawPlayer; }
void setRawPlayer(void* playerPtr) { g_rawPlayer = playerPtr; }

SDL_Renderer* getRawRenderer() { return g_rawRenderer; }
void setRawRenderer(SDL_Renderer* renderer) { g_rawRenderer = renderer; }

SDL_Window* getRawWindow() { return g_rawWindow; }
void setRawWindow(SDL_Window* window) { g_rawWindow = window; }

} // namespace lowlevel
} // namespace sense
