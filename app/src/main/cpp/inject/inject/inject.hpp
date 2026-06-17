#pragma once

#include <vector>
#include <windows.h>
#include <sstream>

std::vector<std::string> getDllFiles(const std::string& folder);
bool createProcess(PROCESS_INFORMATION& pi, const char* path);
LPVOID getLoadLibraryAddr();
LPVOID writeDllPath(HANDLE process, const char* dll_path);
bool injectDLL(PROCESS_INFORMATION& pi);