#include "config.hpp"
#include <fstream>
#include <sstream>
#include <windows.h>

namespace sense {

ConfigFile::ConfigFile(const std::string& filePath) {
    load(filePath);
}

bool ConfigFile::load(const std::string& filePath) {
    m_path = filePath;
    m_values.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);

            // Trim
            while (!key.empty() && isspace(key.front())) key.erase(key.begin());
            while (!key.empty() && isspace(key.back())) key.pop_back();
            while (!val.empty() && isspace(val.front())) val.erase(val.begin());
            while (!val.empty() && isspace(val.back())) val.pop_back();

            m_values[key] = val;
        }
    }
    return true;
}

bool ConfigFile::save(const std::string& filePath) {
    std::string targetPath = filePath.empty() ? m_path : filePath;
    if (targetPath.empty()) return false;

    std::ofstream file(targetPath);
    if (!file.is_open()) return false;

    for (const auto& kv : m_values) {
        file << kv.first << "=" << kv.second << "\n";
    }
    return true;
}

std::string ConfigFile::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_values.find(key);
    return (it != m_values.end()) ? it->second : defaultValue;
}

int ConfigFile::getInt(const std::string& key, int defaultValue) const {
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        try { return std::stoi(it->second); } catch (...) {}
    }
    return defaultValue;
}

float ConfigFile::getFloat(const std::string& key, float defaultValue) const {
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        try { return std::stof(it->second); } catch (...) {}
    }
    return defaultValue;
}

bool ConfigFile::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        return (it->second == "true" || it->second == "1" || it->second == "yes");
    }
    return defaultValue;
}

void ConfigFile::setString(const std::string& key, const std::string& value) { m_values[key] = value; }
void ConfigFile::setInt(const std::string& key, int value) { m_values[key] = std::to_string(value); }
void ConfigFile::setFloat(const std::string& key, float value) { m_values[key] = std::to_string(value); }
void ConfigFile::setBool(const std::string& key, bool value) { m_values[key] = value ? "true" : "false"; }

bool ConfigFile::hasKey(const std::string& key) const {
    return m_values.find(key) != m_values.end();
}

std::string ConfigFile::getModsDirectory() {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash) + "/mods";
    }
    return "mods";
}

std::string ConfigFile::getConfigDirectory() {
    std::string dir = getModsDirectory() + "/config";
    CreateDirectoryA(dir.c_str(), nullptr);
    return dir;
}

} // namespace sense

