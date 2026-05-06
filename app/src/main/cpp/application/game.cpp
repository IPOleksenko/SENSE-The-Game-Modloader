#include <application/game.hpp>
#include <log/logger.hpp>
#include <inject/inject.hpp>

#include <iostream>
#include <windows.h>

Game::Game() : m_isInit(false) {
    m_isInit = true;
}

Game::~Game() {
    m_isInit = false;
}

bool Game::isInit() const {
    return m_isInit;
}

int Game::run() {
    if (!isInit()) return 1;

    PROCESS_INFORMATION pi{};
    if (!createProcess(pi)) return 1;

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