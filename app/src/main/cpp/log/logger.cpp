#include <log/Logger.hpp>

namespace logger {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() = default;

void Logger::setDebug(bool enabled) {
    debugEnabled = enabled;
}

std::string Logger::timeNow() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);

    std::tm tm{};

#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

const char* Logger::levelToString(Level level) {
    switch (level) {
        case Level::Info: return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Debug: return "DEBUG";
    }
    return "UNKNOWN";
}

} // namespace log