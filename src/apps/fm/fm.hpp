#ifndef FM_HPP
#define FM_HPP

#include <string>
#include <vector>
#include "cinput.hpp"

namespace fm {

struct FileItem {
    std::string name;
    int size;
    bool is_dir;
};

class FMListView : public cinput::ListView {
public:
    using cinput::ListView::ListView;
    void draw_item(int x, int y, const cinput::ListItem& item, bool is_selected) override;
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
    std::string sort_mode;
    cinput::Theme theme;
};

} // namespace fm

#endif
