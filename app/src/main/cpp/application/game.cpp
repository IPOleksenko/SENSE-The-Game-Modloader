#include <application/game.hpp>
#include <log/logger.hpp>
#include <inject/inject.hpp>

#include <iostream>
#include <windows.h>
#include <cstring>
#include <vector>
#include <tlhelp32.h>

Game::Game() : m_isInit(true) {}

Game::~Game() { m_isInit = false; }

bool Game::isInit() const {
    return m_isInit;
}

uintptr_t GetProcessBaseAddress(HANDLE hProcess, DWORD processId) {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
            HANDLE, DWORD, PVOID, ULONG, PULONG);

        pNtQueryInformationProcess NtQueryInformationProcess =
            (pNtQueryInformationProcess)GetProcAddress(ntdll, "NtQueryInformationProcess");

        if (NtQueryInformationProcess) {
            struct PROCESS_BASIC_INFORMATION {
                PVOID Reserved1;
                PVOID PebBaseAddress;
                PVOID Reserved2_0;
                PVOID Reserved2_1;
                PVOID UniqueProcessId;
                PVOID Reserved3;
            };

            PROCESS_BASIC_INFORMATION pbi;
            ULONG returnLength;

            if (NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &returnLength) == 0) {
                struct PEB {
                    BOOLEAN InheritedAddressSpace;
                    BOOLEAN ReadImageFileExecOptions;
                    BOOLEAN BeingDebugged;
                    BOOLEAN Spare;
                    PVOID Mutant;
                    PVOID ImageBaseAddress;
                };

                PEB peb;
                if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) {
                    return (uintptr_t)peb.ImageBaseAddress;
                }
            }
        }
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 module;
        module.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(snapshot, &module)) {
            uintptr_t base = (uintptr_t)module.modBaseAddr;
            CloseHandle(snapshot);
            return base;
        }
        CloseHandle(snapshot);
    }

    return 0;
}

bool PatchGameStringDirect(PROCESS_INFORMATION& pi, const char* newText) {
    uintptr_t baseAddr = GetProcessBaseAddress(pi.hProcess, pi.dwProcessId);
    if (!baseAddr) {
        LOG_ERROR("Failed to get process base address");
        return false;
    }

    LOG_INFO(("Game base address: 0x" + std::to_string(baseAddr)).c_str());

    uintptr_t stringAddr = baseAddr + 0x1BE00;

    char currentText[20] = { 0 };
    ReadProcessMemory(pi.hProcess, (LPCVOID)stringAddr, currentText, 15, NULL);

    DWORD oldProtect;
    if (!VirtualProtectEx(pi.hProcess, (LPVOID)stringAddr, 16, PAGE_READWRITE, &oldProtect)) {
        LOG_ERROR("Failed to change memory protection");
        return false;
    }

    size_t newLen = strlen(newText);
    if (newLen > 15) newLen = 15;

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(pi.hProcess, (LPVOID)stringAddr, newText, newLen + 1, &bytesWritten)) {
        LOG_ERROR("Failed to write memory");
        VirtualProtectEx(pi.hProcess, (LPVOID)stringAddr, 16, oldProtect, &oldProtect);
        return false;
    }

    VirtualProtectEx(pi.hProcess, (LPVOID)stringAddr, 16, oldProtect, &oldProtect);

    char newCurrent[20] = { 0 };
    ReadProcessMemory(pi.hProcess, (LPCVOID)stringAddr, newCurrent, 15, NULL);

    return true;
}


int Game::run() {
    if (!isInit()) return 1;

    PROCESS_INFORMATION pi{};
    if (!createProcess(pi)) return 1;

    bool patchStringSuccess = PatchGameStringDirect(pi, "Moded");

    bool injectionResult = injectDLL(pi);
    if (!injectionResult) {
        LOG_ERROR("Injection failed for all DLLs.");
        TerminateProcess(pi.hProcess, 1);
    }
    else {
        ResumeThread(pi.hThread);
        LOG_INFO("Main thread resumed. Game is running with injected mods.");
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return injectionResult ? 0 : 1;
}