#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <MinHook.h>

// Global handle for the real SDL2.dll
HMODULE hRealSDL2 = nullptr;

// Function to load the real SDL2.dll
bool LoadRealSDL2() {
    if (hRealSDL2) return true;
    
    hRealSDL2 = LoadLibraryA("SDL2_original.dll");
    
    if (!hRealSDL2) {
        // Log error to a file for diagnostics
        HANDLE hFile = CreateFileA("proxy_error.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            const char* msg = "Failed to load SDL2_original.dll\nError code: ";
            DWORD written;
            WriteFile(hFile, msg, (DWORD)strlen(msg), &written, NULL);
            std::string code = std::to_string(GetLastError());
            WriteFile(hFile, code.c_str(), (DWORD)code.length(), &written, NULL);
            CloseHandle(hFile);
        }

        MessageBoxA(NULL, "SENSE Modloader: Failed to load SDL2_original.dll!\nMake sure the original SDL2.dll was renamed to SDL2_original.dll.", "Error", MB_ICONERROR);
        return false;
    }
    return true;
}

// Exports are now handled by proxy.def

// Modding logic
void InitializeMod() {
    if (MH_Initialize() != MH_OK) {
        return;
    }

    // MH_CreateHook(...);
    // MH_EnableHook(...);

    std::cout << "Initialized" << std::endl;
}

DWORD WINAPI ModThread(LPVOID lpParam) {
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    std::cout << "SENSE: The Game Modloader" << std::endl;
    std::cout << std::endl << std::endl;
    InitializeMod();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if (LoadRealSDL2()) {
            CreateThread(NULL, 0, ModThread, NULL, 0, NULL);
        }
        break;
    case DLL_PROCESS_DETACH:
        if (hRealSDL2) {
            FreeLibrary(hRealSDL2);
        }
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
