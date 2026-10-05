#include <inject/inject.hpp>
#include <log/logger.hpp>
#include <algorithm>

static std::string toAbsolutePath(const std::string& path) {
    char fullPath[MAX_PATH] = {};
    if (GetFullPathNameA(path.c_str(), MAX_PATH, fullPath, nullptr)) {
        return std::string(fullPath);
    }
    return path;
}

std::vector<std::string> getDllFiles(const std::string& folder) {
    std::vector<std::string> result;
    std::string searchPath = folder + "\\*.dll";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return result;
    }

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // Ignore core engine / library DLLs if placed in mods folder
            if (_stricmp(fd.cFileName, "SENSE_THE_GAME_MODLOADER_api.dll") == 0 ||
                _stricmp(fd.cFileName, "steam_api64.dll") == 0 ||
                _stricmp(fd.cFileName, "SDL2.dll") == 0 ||
                _stricmp(fd.cFileName, "SDL2_image.dll") == 0 ||
                _stricmp(fd.cFileName, "SDL2_mixer.dll") == 0 ||
                _stricmp(fd.cFileName, "SDL2_ttf.dll") == 0) {
                continue;
            }
            result.push_back(toAbsolutePath(folder + "\\" + fd.cFileName));
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return result;
}

bool createProcess(PROCESS_INFORMATION& pi, const char* path) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(
        path, NULL, NULL, NULL, FALSE,
        CREATE_SUSPENDED, NULL, NULL, &si, &pi
    );

    if (!success) {
        LOG_ERROR("CreateProcessA");
        return false;
    }
    LOG_INFO("Target process created suspended.");
    return true;
}

LPVOID getLoadLibraryAddr() {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        LOG_ERROR("GetModuleHandleA(kernel32)");
        return nullptr;
    }
    FARPROC addr = GetProcAddress(kernel32, "LoadLibraryA");
    if (!addr) {
        LOG_ERROR("GetProcAddress(LoadLibraryA)");
        return nullptr;
    }
    return (LPVOID)addr;
}

LPVOID writeDllPath(HANDLE process, const char* dll_path) {
    size_t size = strlen(dll_path) + 1;
    LPVOID remoteMem = VirtualAllocEx(process, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        LOG_ERROR("VirtualAllocEx");
        return nullptr;
    }
    if (!WriteProcessMemory(process, remoteMem, dll_path, size, NULL)) {
        LOG_ERROR("WriteProcessMemory");
        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        return nullptr;
    }
    return remoteMem;
}

static bool injectSingleDll(PROCESS_INFORMATION& pi, LPVOID loadLib, const std::string& dllPath, bool isDependency = false) {
    std::string absPath = toAbsolutePath(dllPath);

    LPVOID remoteMem = writeDllPath(pi.hProcess, absPath.c_str());
    if (!remoteMem) {
        LOG_WARN(("Failed to write DLL path into target process: " + absPath).c_str());
        return false;
    }

    HANDLE hRemoteThread = CreateRemoteThread(
        pi.hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadLib,
        remoteMem, 0, NULL
    );

    if (!hRemoteThread) {
        LOG_ERROR("CreateRemoteThread");
        VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(hRemoteThread, INFINITE);

    DWORD exitCode = 0;
    bool success = false;
    if (GetExitCodeThread(hRemoteThread, &exitCode)) {
        if (exitCode != 0) {
            success = true;
            if (!isDependency) {
                LOG_INFO(("Success! DLL loaded, handle = 0x" + std::to_string(exitCode)).c_str());
            }
        } else {
            if (!isDependency) {
                LOG_INFO("LoadLibrary returned NULL – injection failed (DLL likely missing dependencies or wrong architecture).");
            }
        }
    }

    CloseHandle(hRemoteThread);
    VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
    return success;
}

bool injectDLL(PROCESS_INFORMATION& pi) {
    LPVOID loadLib = getLoadLibraryAddr();
    if (!loadLib) {
        LOG_WARN("Cannot get LoadLibraryA address.");
        return false;
    }

    // Determine launcher directory
    char launcherDir[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, launcherDir, MAX_PATH);
    char* lastSlash = strrchr(launcherDir, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';

    // 1. Pre-load steam_api64.dll into game process if present
    std::string steamCandidates[] = {
        std::string(launcherDir) + "steam_api64.dll",
        toAbsolutePath("steam_api64.dll"),
        toAbsolutePath("mods\\steam_api64.dll")
    };
    for (const auto& candidate : steamCandidates) {
        WIN32_FIND_DATAA fd = {};
        HANDLE h = FindFirstFileA(candidate.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            LOG_INFO(("Pre-loading Steam dependency: " + candidate).c_str());
            injectSingleDll(pi, loadLib, candidate, true);
            break;
        }
    }

    // 2. Pre-load shared API library SENSE_THE_GAME_MODLOADER_api.dll by absolute path
    std::string apiCandidates[] = {
        std::string(launcherDir) + "SENSE_THE_GAME_MODLOADER_api.dll",
        toAbsolutePath("SENSE_THE_GAME_MODLOADER_api.dll"),
        toAbsolutePath("mods\\SENSE_THE_GAME_MODLOADER_api.dll")
    };
    bool apiLoaded = false;
    for (const auto& candidate : apiCandidates) {
        WIN32_FIND_DATAA fd = {};
        HANDLE h = FindFirstFileA(candidate.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
            LOG_INFO(("Pre-loading Core API DLL: " + candidate).c_str());
            apiLoaded = injectSingleDll(pi, loadLib, candidate, true);
            if (apiLoaded) {
                LOG_INFO("Core API DLL loaded into game successfully.");
            } else {
                LOG_WARN("Core API DLL failed to load into game.");
            }
            break;
        }
    }

    if (!apiLoaded) {
        LOG_WARN("Core API DLL not found before mod injection! Trying to proceed...");
    }

    // 3. Inject all mods by absolute path
    auto dlls = getDllFiles("mods");
    if (dlls.empty()) {
        LOG_WARN("No DLLs found in 'mods' folder.");
        return false;
    }

    LOG_INFO(("Found " + std::to_string(dlls.size()) + " DLL(s):").c_str());
    for (const auto& path : dlls) {
        LOG_INFO((" - " + path).c_str());
    }
    LOG_INFO("Starting injection...");

    bool anySuccess = false;
    for (const auto& dll_path : dlls) {
        LOG_INFO(("Injecting: " + dll_path).c_str());
        if (injectSingleDll(pi, loadLib, dll_path, false)) {
            anySuccess = true;
        }
    }

    return anySuccess;
}