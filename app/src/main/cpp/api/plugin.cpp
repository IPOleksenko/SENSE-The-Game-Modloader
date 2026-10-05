#include "plugin.hpp"
#include <iostream>

namespace sense {

// ============================================================================
// NativeLibrary Implementation
// ============================================================================
NativeLibrary::NativeLibrary() = default;

NativeLibrary::NativeLibrary(const std::string& dllPath) {
    load(dllPath);
}

NativeLibrary::~NativeLibrary() {
    free();
}

NativeLibrary::NativeLibrary(NativeLibrary&& other) noexcept
    : m_handle(other.m_handle), m_path(std::move(other.m_path)), m_ownsHandle(other.m_ownsHandle) {
    other.m_handle = nullptr;
}

NativeLibrary& NativeLibrary::operator=(NativeLibrary&& other) noexcept {
    if (this != &other) {
        free();
        m_handle = other.m_handle;
        m_path = std::move(other.m_path);
        m_ownsHandle = other.m_ownsHandle;
        other.m_handle = nullptr;
    }
    return *this;
}

bool NativeLibrary::load(const std::string& dllPath) {
    free();
    m_path = dllPath;

    // Check if already loaded in process
    m_handle = GetModuleHandleA(dllPath.c_str());
    if (m_handle) {
        m_ownsHandle = false;
        return true;
    }

    m_handle = LoadLibraryA(dllPath.c_str());
    m_ownsHandle = (m_handle != nullptr);
    return m_handle != nullptr;
}

void NativeLibrary::free() {
    if (m_handle && m_ownsHandle) {
        FreeLibrary(m_handle);
    }
    m_handle = nullptr;
    m_path.clear();
}

bool NativeLibrary::isLoaded() const {
    return m_handle != nullptr;
}

void* NativeLibrary::getSymbol(const std::string& symbolName) const {
    if (!m_handle) return nullptr;
    return reinterpret_cast<void*>(GetProcAddress(m_handle, symbolName.c_str()));
}

// ============================================================================
// PluginManager Implementation
// ============================================================================
PluginManager& PluginManager::instance() {
    static PluginManager mgr;
    return mgr;
}

std::shared_ptr<NativeLibrary> PluginManager::load(const std::string& name, const std::string& dllPath) {
    auto lib = std::make_shared<NativeLibrary>();
    if (lib->load(dllPath)) {
        m_libraries[name] = lib;
        return lib;
    }
    return nullptr;
}

std::shared_ptr<NativeLibrary> PluginManager::get(const std::string& name) {
    auto it = m_libraries.find(name);
    if (it != m_libraries.end()) {
        return it->second;
    }
    return nullptr;
}

bool PluginManager::unload(const std::string& name) {
    auto it = m_libraries.find(name);
    if (it != m_libraries.end()) {
        m_libraries.erase(it);
        return true;
    }
    return false;
}

bool PluginManager::isLoaded(const std::string& name) const {
    return m_libraries.find(name) != m_libraries.end();
}

size_t PluginManager::scanAndLoadDirectory(const std::string& directoryPath) {
    size_t loadedCount = 0;
    std::string searchPattern = directoryPath;
    if (searchPattern.empty()) searchPattern = ".";
    if (searchPattern.back() != '\\' && searchPattern.back() != '/') {
        searchPattern += "/";
    }
    searchPattern += "*.dll";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findData.cFileName;
                std::string fullPath = directoryPath;
                if (fullPath.back() != '\\' && fullPath.back() != '/') {
                    fullPath += "/";
                }
                fullPath += filename;

                std::string modName = filename;
                if (modName.size() > 4 && modName.substr(modName.size() - 4) == ".dll") {
                    modName = modName.substr(0, modName.size() - 4);
                }

                if (load(modName, fullPath)) {
                    loadedCount++;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    return loadedCount;
}

} // namespace sense

