#ifndef FM_HPP
#define FM_HPP

#include <string>
#include <vector>
#include "cinput.hpp"

namespace fm {

struct FileItem {
    std::string name;
    uint32_t size;
    bool is_dir;
};

class FileManager {
public:
    FileManager();
    void run();

private:
    void refresh();
    void draw_header();
    void show_preview(const FileItem& item);
    void show_hex_preview(const FileItem& item);
    void show_text_preview(const FileItem& item);

    std::string current_path;
    std::vector<FileItem> items;
    std::string filter;
    std::string sort_mode; // "name", "size"
    cinput::Theme theme;
};

} // namespace fm

#endif
