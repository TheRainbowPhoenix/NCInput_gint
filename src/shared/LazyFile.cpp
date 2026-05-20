#include "LazyFile.hpp"
#include <cstring>
#include <cerrno>

LazyFile::LazyFile(const std::string& path, const std::string& mode)
    : m_path(path), m_mode(mode) {}

LazyFile::shared_type LazyFile::acquire() {
    if (auto shared = m_file.lock())
        return shared;

    std::FILE* raw = std::fopen(m_path.c_str(), m_mode.c_str());
    if (!raw) return nullptr;

    auto shared = shared_type(raw, [](std::FILE* file) {
        if (file) std::fclose(file);
    });

    m_file = shared;
    return shared;
}
