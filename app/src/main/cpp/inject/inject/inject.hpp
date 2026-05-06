#pragma once

#include <vector>
#include <windows.h>
#include <sstream>

bool createProcess(PROCESS_INFORMATION& pi);
bool injectDLL(PROCESS_INFORMATION& pi);