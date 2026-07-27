#pragma once

#include <cstdint>
#include <windows.h>

class Game {
public:
    Game();
    ~Game();

    bool isInit() const;
    bool run();

private:
    bool m_isInit;

    const char* TARGET_PATH = "SENSE_THE_GAME.exe";
    
    PROCESS_INFORMATION pi{};

    uintptr_t baseAddr;
    uintptr_t stringAddr;
    uintptr_t stringOffset = 0x3EC20; // "by IPOleksenko"

    bool patchStringSuccess;
    bool injectionResult;

    uintptr_t GetProcessBaseAddress(HANDLE hProcess, DWORD processId);
    bool PatchGameStringDirect(PROCESS_INFORMATION& pi, const char* newText);
};