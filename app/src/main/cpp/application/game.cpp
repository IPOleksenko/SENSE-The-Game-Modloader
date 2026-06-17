#include <application/game.hpp>
#include <log/logger.hpp>
#include <inject/inject.hpp>

#include <iostream>
#include <cstring>
#include <vector>
#include <tlhelp32.h>

Game::Game() : m_isInit(true) {}

Game::~Game() {
    if (pi.hThread) CloseHandle(pi.hThread);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    
    m_isInit = false; 
}

bool Game::isInit() const {
    return m_isInit;
}

uintptr_t Game::GetProcessBaseAddress(HANDLE hProcess, DWORD processId) {
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

bool Game::PatchGameStringDirect(PROCESS_INFORMATION& pi, const char* newText) {
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


bool Game::run() {
    if (!isInit()) return EXIT_FAILURE;

    if (!createProcess(pi, TARGET_PATH)) return EXIT_FAILURE;

    // Get base address before process creation to ensure we have the correct handle and process ID
    baseAddr = GetProcessBaseAddress(pi.hProcess, pi.dwProcessId);
    if (!baseAddr) {
        LOG_ERROR("Failed to get process base address");
        return EXIT_FAILURE;
    } LOG_INFO(("Game base address: 0x" + std::to_string(baseAddr)).c_str());
    
    // Calculate string address using the base address and offset
    stringAddr = baseAddr + stringOffset; 
    LOG_INFO(("Target string address: 0x" + std::to_string(stringAddr)).c_str());

    // Inject DLLs into the target process
    injectionResult = injectDLL(pi);
    if (!injectionResult) {
        LOG_WARN("Injection failed for all DLLs or DLLs not found.");
        patchStringSuccess = PatchGameStringDirect(pi, "No Injected DLLs");
    }
    else {
        patchStringSuccess = PatchGameStringDirect(pi, "Injected");
        if (!patchStringSuccess) {
            LOG_ERROR("Failed to patch the game string.");
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return EXIT_FAILURE;
        }
    }

    ResumeThread(pi.hThread);

    bool ok = injectionResult && patchStringSuccess;
    LOG_INFO(ok
        ? "Main thread resumed. Game is running with injected mods."
        : "Main thread resumed. Game is running without injected mods.");

    WaitForSingleObject(pi.hProcess, INFINITE);

    return EXIT_SUCCESS;
}