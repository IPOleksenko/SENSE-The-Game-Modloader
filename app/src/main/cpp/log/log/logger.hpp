#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <string>

namespace logger {

enum class Level {
    Info,
    Warning,
    Error,
    Debug
};

class Logger {
public:
    static Logger& instance();

    void setDebug(bool enabled);

    template<typename... Args>
    void log(Level level, Args&&... args) {
        if (level == Level::Debug && !debugEnabled)
            return;

        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;
        (oss << ... << args);

        std::cout
            << "[" << timeNow() << "] "
            << "[" << levelToString(level) << "] "
            << oss.str()
            << std::endl;
    }

private:
    Logger();

    std::string timeNow();
    const char* levelToString(Level level);

private:
    std::mutex mutex_;
    bool debugEnabled = false;
};

} // namespace logger

#define LOG_INFO(...)  logger::Logger::instance().log(logger::Level::Info, __VA_ARGS__)
#define LOG_WARN(...)  logger::Logger::instance().log(logger::Level::Warning, __VA_ARGS__)
#define LOG_ERROR(...) logger::Logger::instance().log(logger::Level::Error, __VA_ARGS__)
#define LOG_DEBUG(...) logger::Logger::instance().log(logger::Level::Debug, __VA_ARGS__)