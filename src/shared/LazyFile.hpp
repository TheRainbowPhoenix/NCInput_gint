#pragma once
#include <cstdio>
#include <memory>
#include <string>

class LazyFile {
public:
    using shared_type = std::shared_ptr<std::FILE>;
    explicit LazyFile(const std::string& path, const std::string& mode);
    shared_type acquire();

private:
    std::string m_path;
    std::string m_mode;
    std::weak_ptr<std::FILE> m_file;
};
