#pragma once

#include "types.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

namespace sense {

// ============================================================================
// 1. Native Dynamic Library Wrapper (LoadLibrary / GetProcAddress)
// ============================================================================
class NativeLibrary {
public:
    NativeLibrary();
    explicit NativeLibrary(const std::string& dllPath);
    ~NativeLibrary();

    NativeLibrary(const NativeLibrary&) = delete;
    NativeLibrary& operator=(const NativeLibrary&) = delete;

    NativeLibrary(NativeLibrary&& other) noexcept;
    NativeLibrary& operator=(NativeLibrary&& other) noexcept;

    bool load(const std::string& dllPath);
    void free();
    bool isLoaded() const;

    void* getSymbol(const std::string& symbolName) const;

    template<typename FuncType>
    FuncType* getFunction(const std::string& symbolName) const {
        return reinterpret_cast<FuncType*>(getSymbol(symbolName));
    }

    template<typename Ret, typename... Args>
    Ret invoke(const std::string& symbolName, Args... args) {
        auto fn = getFunction<Ret(*)(Args...)>(symbolName);
        if (!fn) {
            throw std::runtime_error("Symbol not found: " + symbolName);
        }
        return fn(args...);
    }

    HMODULE getHandle() const { return m_handle; }
    const std::string& getPath() const { return m_path; }

private:
    HMODULE m_handle = nullptr;
    std::string m_path;
    bool m_ownsHandle = true;
};

// ============================================================================
// 2. Plugin Manager (Inter-DLL Management & Foreign Plugin Loader)
// ============================================================================
class PluginManager {
public:
    static PluginManager& instance();

    // Loads arbitrary native DLL into game address space
    std::shared_ptr<NativeLibrary> load(const std::string& name, const std::string& dllPath);
    std::shared_ptr<NativeLibrary> get(const std::string& name);
    bool unload(const std::string& name);
    bool isLoaded(const std::string& name) const;

    // Scan a directory and load all .dll files
    size_t scanAndLoadDirectory(const std::string& directoryPath);

private:
    PluginManager() = default;
    std::unordered_map<std::string, std::shared_ptr<NativeLibrary>> m_libraries;
};

// Convenience namespace functions
namespace plugins {
    inline std::shared_ptr<NativeLibrary> load(const std::string& name, const std::string& path) {
        return PluginManager::instance().load(name, path);
    }
    inline std::shared_ptr<NativeLibrary> get(const std::string& name) {
        return PluginManager::instance().get(name);
    }
    inline bool unload(const std::string& name) {
        return PluginManager::instance().unload(name);
    }
    inline size_t scan(const std::string& directory) {
        return PluginManager::instance().scanAndLoadDirectory(directory);
    }
}

} // namespace sense

