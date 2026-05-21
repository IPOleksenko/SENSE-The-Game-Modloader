#include <inject/inject.hpp>
#include <log/logger.hpp>

const char* TARGET_PATH = "SENSE_THE_GAME.exe";

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
            result.push_back(folder + "\\" + fd.cFileName);
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return result;
}

bool createProcess(PROCESS_INFORMATION& pi) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(
        TARGET_PATH, NULL, NULL, NULL, FALSE,
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

bool injectDLL(PROCESS_INFORMATION& pi) {
    LPVOID loadLib = getLoadLibraryAddr();
    if (!loadLib) {
        LOG_WARN("Cannot get LoadLibraryA address.");
        return false;
    }

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

        LPVOID remoteMem = writeDllPath(pi.hProcess, dll_path.c_str());
        if (!remoteMem) {
            LOG_WARN("Failed to write DLL path into target process.");
            continue;
        }

        HANDLE hRemoteThread = CreateRemoteThread(
            pi.hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)loadLib,
            remoteMem, 0, NULL
        );

        if (!hRemoteThread) {
            LOG_ERROR("CreateRemoteThread");
            VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
            continue;
        }

        WaitForSingleObject(hRemoteThread, INFINITE);

        DWORD exitCode = 0;
        if (!GetExitCodeThread(hRemoteThread, &exitCode)) {
            LOG_ERROR("GetExitCodeThread");
        }
        else {
            if (exitCode != 0) {
                LOG_INFO(("Success! DLL loaded, handle = 0x" + std::to_string(exitCode)).c_str());
                anySuccess = true;
            }
            else {
                LOG_INFO("LoadLibrary returned NULL – injection failed (DLL likely missing dependencies or wrong architecture).");
            }
        }

        CloseHandle(hRemoteThread);
        VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
    }

    return anySuccess;
}