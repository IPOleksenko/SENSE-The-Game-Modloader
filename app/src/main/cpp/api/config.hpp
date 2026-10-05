#pragma once

#include <string>
#include <map>

namespace sense {

class ConfigFile {
public:
    ConfigFile() = default;
    explicit ConfigFile(const std::string& filePath);

    bool load(const std::string& filePath);
    bool save(const std::string& filePath = "");

    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);

    bool hasKey(const std::string& key) const;

    static std::string getModsDirectory();
    static std::string getConfigDirectory();

private:
    std::string m_path;
    std::map<std::string, std::string> m_values;
};

} // namespace sense

