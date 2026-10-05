#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace sense {
namespace lowlevel {

// ============================================================================
// 1. x64 Inline Detour Hook Engine
// ============================================================================
class DetourHook {
public:
    DetourHook();
    ~DetourHook();

    // Creates an absolute 64-bit detour jump (14 bytes: FF 25 00 00 00 00 [64-bit addr])
    bool create(void* targetFunction, void* detourFunction, void** outOriginalTrampoline);
    bool enable();
    bool disable();
    void remove();

    bool isEnabled() const { return m_enabled; }
    void* getTarget() const { return m_target; }
    void* getTrampoline() const { return m_trampoline; }

private:
    void* m_target = nullptr;
    void* m_detour = nullptr;
    void* m_trampoline = nullptr;
    size_t m_patchSize = 14;
    std::vector<uint8_t> m_originalBytes;
    bool m_enabled = false;
};

// ============================================================================
// 2. VMT (Virtual Method Table) Hooking
// ============================================================================
bool hookVirtualMethod(void* classInstance, size_t methodIndex, void* newMethod, void** outOriginalMethod);

// ============================================================================
// 3. Memory Reading, Writing & Patching
// ============================================================================
bool protect(uintptr_t address, size_t size, DWORD newProtect, DWORD* outOldProtect = nullptr);
bool readBytes(uintptr_t address, void* buffer, size_t size);
bool writeBytes(uintptr_t address, const void* buffer, size_t size);
bool patch(uintptr_t address, const std::vector<uint8_t>& bytes);
bool nop(uintptr_t address, size_t count);

template<typename T>
T read(uintptr_t address, const T& defaultValue = T()) {
    T value = defaultValue;
    if (readBytes(address, &value, sizeof(T))) {
        return value;
    }
    return defaultValue;
}

template<typename T>
bool write(uintptr_t address, const T& value) {
    return writeBytes(address, &value, sizeof(T));
}

// ============================================================================
// 4. Pattern / Signature Scanner (AOB)
// ============================================================================
uintptr_t findPattern(const char* moduleName, const char* pattern, const char* mask);
uintptr_t scanMemory(uintptr_t startAddress, size_t searchSize, const char* pattern, const char* mask);
uintptr_t findSignature(const char* moduleName, const std::string& idaSignature);
uintptr_t getModuleBase(const char* moduleName = nullptr);
size_t getModuleSize(const char* moduleName = nullptr);

// ============================================================================
// 5. Win32 Window Subclassing & Manipulation
// ============================================================================
HWND getWindowHandle();
void setWindowHandle(HWND hwnd);

using WndProcCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
bool hookWndProc(WndProcCallback newWndProc);
LRESULT callOriginalWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void setWindowTransparency(uint8_t alpha);
void setWindowTitle(const std::string& title);
void setWindowBorderless(bool borderless);
void setWindowPosition(int x, int y, int width, int height);

// ============================================================================
// 6. Raw Game Memory Offsets (SENSE: The Game)
// ============================================================================
constexpr uintptr_t OFFSET_PLAYER_TEXTURE        = 0x00; // RawTexture (sizeof 0x28)
constexpr uintptr_t OFFSET_PLAYER_POS_Y          = 0x28; // int
constexpr uintptr_t OFFSET_PLAYER_SPEED          = 0x2C; // float
constexpr uintptr_t OFFSET_PLAYER_LAST_MOVE      = 0x30; // int (PlayerMove)
constexpr uintptr_t OFFSET_PLAYER_IS_MOVING      = 0x34; // bool
constexpr uintptr_t OFFSET_PLAYER_HAS_LOST       = 0x35; // bool
constexpr uintptr_t OFFSET_PLAYER_SPEED_SCALE    = 0x38; // float (5.0f base)
constexpr uintptr_t OFFSET_PLAYER_SPEED_DECREASE = 0x3C; // float (0.5f base)
constexpr uintptr_t OFFSET_PLAYER_SPEED_MIN      = 0x40; // const float (50.5f)
constexpr uintptr_t OFFSET_PLAYER_SPEED_MAX      = 0x44; // const float (150.0f)
constexpr uintptr_t OFFSET_PLAYER_SPEED_NORMAL   = 0x48; // const float (100.25f)
constexpr uintptr_t OFFSET_PLAYER_FRAME_SIZE_X   = 0x4C; // int
constexpr uintptr_t OFFSET_PLAYER_FRAME_SIZE_Y   = 0x50; // int
constexpr uintptr_t OFFSET_PLAYER_FRAME_CURRENT  = 0x54; // int
constexpr uintptr_t OFFSET_PLAYER_LAST_FRAME_TIME= 0x58; // Uint32
constexpr uintptr_t OFFSET_PLAYER_FRAMES_TOTAL   = 0x5C; // const int (120)
constexpr uintptr_t OFFSET_PLAYER_FRAMES_PER_ROW = 0x60; // const int (20)
constexpr uintptr_t OFFSET_PLAYER_ANIM_SPEED     = 0x64; // const int (100)
constexpr uintptr_t OFFSET_PLAYER_FINAL_ANIM_DONE= 0x68; // bool

// Live raw pointers
void* getRawPlayer();
void setRawPlayer(void* playerPtr);

SDL_Renderer* getRawRenderer();
void setRawRenderer(SDL_Renderer* renderer);

SDL_Window* getRawWindow();
void setRawWindow(SDL_Window* window);

} // namespace lowlevel
} // namespace sense

