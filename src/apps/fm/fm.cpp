#include "fm.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <sys/stat.h>

namespace fm {

class FMListView : public cinput::ListView {
public:
    using cinput::ListView::ListView;

    void draw_item(int x, int y, const cinput::ListItem& item, bool is_selected) override {
        const auto& t = m_theme;
        int h = item._h;

        color_t bg = is_selected ? t.hl : t.modal_bg;
        drect(x, y, x + m_rect.w, y + h, bg);
        ::drect_border(x, y, x + m_rect.w, y + h, (int)C_NONE, 1, (int)t.key_spec);

        color_t col = t.txt;
        color_t icon_col = is_selected ? t.txt : t.accent;

        if (item.type == "folder") {
            // Breeze style Folder Icon - FILLED
            color_t inner_col = t.txt_dim;
            int px1[] = {x+8, x+16, x+20, x+20, x+8};
            int py1[] = {y+12, y+12, y+16, y+22, y+22};
            dpoly(px1, py1, 5, (int)inner_col, (int)inner_col);

            int px2[] = {x+8, x+14, x+19, x+32, x+32, x+8};
            int py2[] = {y+21, y+21, y+16, y+16, y+35, y+35};
            dpoly(px2, py2, 6, (int)icon_col, (int)icon_col);
        } else {
            // File Icon
            int px1[] = {x+12, x+22, x+28, x+28, x+12};
            int py1[] = {y+14, y+14, y+20, y+36, y+36};
            dpoly(px1, py1, 5, (int)C_NONE, (int)icon_col);

            int px2[] = {x+22, x+22, x+28};
            int py2[] = {y+14, y+20, y+20};
            dpoly(px2, py2, 3, (int)C_NONE, (int)icon_col);

            dline(x+15, y+24, x+25, y+24, icon_col);
            dline(x+15, y+29, x+25, y+29, icon_col);
            dline(x+15, y+33, x+20, y+33, icon_col);
        }

        dtext_opt(x + 42, y + h / 2, col, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, item.text.c_str(), -1);
    }
};

FileManager::FileManager() : theme(cinput::get_theme("light")) {
    current_path = "/";
    sort_mode = "name";
    refresh();
}

void FileManager::refresh() {
    items.clear();

#ifndef _WIN32
    DIR* dr = opendir(current_path.c_str());
    if (dr) {
        struct dirent* de;
        if (current_path != "/") {
            items.push_back({"..", 0, true});
        }
        while ((de = readdir(dr)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

            FileItem it;
            it.name = de->d_name;
            it.is_dir = (de->d_type == DT_DIR);

            std::string full = current_path;
            if (full.back() != '/') full += '/';
            full += de->d_name;

            struct stat st;
            if (stat(full.c_str(), &st) == 0) {
                it.size = (uint32_t)st.st_size;
                if (S_ISDIR(st.st_mode)) it.is_dir = true;
            } else {
                it.size = 0;
            }
            items.push_back(it);
        }
        closedir(dr);
    }
#else
    // Mock files for Windows build
    if (current_path != "/") items.push_back({"..", 0, true});
    items.push_back({"win_mock_folder", 0, true});
    items.push_back({"readme_win.txt", 512, false});
    items.push_back({"run.bin", 2048, false});
#endif

    if (sort_mode == "name") {
        std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            if (a.name == "..") return true;
            if (b.name == "..") return false;
            if (a.is_dir != b.is_dir) return a.is_dir;
            return a.name < b.name;
        });
    } else {
         std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            if (a.name == "..") return true;
            if (b.name == "..") return false;
            return a.size > b.size;
        });
    }
}

void FileManager::draw_header() {
    drect(0, 0, SCREEN_W, 45, theme.accent);
    dtext_opt(SCREEN_W / 2, 22, theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, current_path.c_str(), -1);
}

void FileManager::show_preview(const FileItem& item) {
    std::vector<std::string> opts = { "Hex View", "Text Preview", "Properties" };
    if (item.name != "run.bin" && item.name != ".." && item.name != "system") {
        opts.push_back("Delete");
    }
    cinput::PickResult res = cinput::pick(opts, item.name, "light");
    if (!res.success || res.values.empty()) return;

    if (res.values[0] == "Hex View") show_hex_preview(item);
    else if (res.values[0] == "Text Preview") show_text_preview(item);
}

void FileManager::show_hex_preview(const FileItem& item) {
    dclear(C_WHITE);
    drect(0, 0, SCREEN_W, 40, theme.accent);
    dtext_opt(SCREEN_W/2, 20, theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Hex View", -1);

    for(int i=0; i<10; ++i) {
        char buf[64];
        sprintf(buf, "%04X: 00 11 22 33 44 55 66 77", i*8);
        dtext(10, 60 + i*20, C_BLACK, buf);
    }
    dupdate();
    while(!keypressed(KEY_EXIT) && !keypressed(KEY_EXE)) {
        pollevent(); sleep_ms(20);
    }
}

void FileManager::show_text_preview(const FileItem& item) {
    dclear(C_WHITE);
    drect(0, 0, SCREEN_W, 40, theme.accent);
    dtext_opt(SCREEN_W/2, 20, theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Text View", -1);
    dtext(10, 60, C_BLACK, "Sample content for");
    dtext(10, 80, C_BLACK, item.name.c_str());
    dupdate();
    while(!keypressed(KEY_EXIT) && !keypressed(KEY_EXE)) {
        pollevent(); sleep_ms(20);
    }
}

void FileManager::run() {
    bool in_main_loop = true;
    while(in_main_loop) {
        std::vector<cinput::ListItem> lv_items;
        for(const auto& it : items) {
            cinput::ListItem li;
            li.text = it.name + (it.is_dir ? "/" : "");
            li.type = it.is_dir ? "folder" : "file";
            lv_items.push_back(li);
        }

        FMListView lv({0, 45, SCREEN_W, SCREEN_H - 45}, lv_items, 50, "light");
        bool in_view = true;

        while(in_view) {
            dclear(theme.modal_bg);
            lv.draw();
            draw_header();
            dupdate();
            cleareventflips();

            if (keypressed(KEY_EXIT)) {
                in_main_loop = false;
                in_view = false;
                break;
            }

            key_event_t ev = pollevent();
            std::vector<key_event_t> events;
            while(ev.type != KEYEV_NONE) {
                events.push_back(ev);
                ev = pollevent();
            }

            cinput::ListView::Action action;
            if (lv.update(events, action)) {
                if (action.type == "click") {
                    const auto& item = items[action.index];
                    if (item.is_dir) {
                        if (item.name == "..") {
                            if (current_path != "/") {
                                size_t last = current_path.find_last_of('/', current_path.length()-2);
                                if (last == std::string::npos) current_path = "/";
                                else current_path = current_path.substr(0, last + 1);
                                refresh();
                                in_view = false;
                            }
                        } else {
                            if (current_path.back() != '/') current_path += "/";
                            current_path += item.name + "/";
                            refresh();
                            in_view = false;
                        }
                    } else {
                        show_preview(item);
                        clearevents();
                    }
                }
            }
            sleep_ms(20);
        }
    }
}

} // namespace fm
