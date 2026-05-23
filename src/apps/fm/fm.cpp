#include "fm.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace fm {

FileManager::FileManager() : theme(cinput::get_theme("dark")) {
    current_path = "\\\\fls0\\";
    sort_mode = "name";
    refresh();
}

void FileManager::refresh() {
    items.clear();
    // In a real gint app, you'd use BFile_FindFirst/Next here.
    // For this demo port, we mock some files.
    items.push_back({"..", 0, true});
    items.push_back({"system", 0, true});
    items.push_back({"main.py", 1024, false});
    items.push_back({"notes.txt", 512, false});
    items.push_back({"data.bin", 4096, false});
    items.push_back({"config.cfg", 128, false});

    if (sort_mode == "name") {
        std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            if (a.is_dir != b.is_dir) return a.is_dir;
            return a.name < b.name;
        });
    } else {
         std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            return a.size > b.size;
        });
    }
}

void FileManager::draw_header() {
    drect(0, 0, SCREEN_W, 45, theme.accent);
    dtext_opt(SCREEN_W / 2, 22, theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, current_path.c_str(), -1);
}

void FileManager::show_preview(const FileItem& item) {
    std::vector<std::string> opts = { "Hex View", "Text Preview", "Properties", "Delete" };
    cinput::PickResult res = cinput::pick(opts, item.name, "dark");
    if (!res.success || res.values.empty()) return;

    if (res.values[0] == "Hex View") show_hex_preview(item);
    else if (res.values[0] == "Text Preview") show_text_preview(item);
}

void FileManager::show_hex_preview(const FileItem& item) {
    dclear(C_BLACK);
    drect(0, 0, SCREEN_W, 40, theme.accent);
    dtext_opt(SCREEN_W/2, 20, theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Hex View", -1);

    for(int i=0; i<10; ++i) {
        char buf[64];
        sprintf(buf, "%04X: 00 11 22 33 44 55 66 77", i*8);
        dtext(10, 60 + i*20, C_WHITE, buf);
    }
    dupdate();
    while(!keypressed(KEY_EXIT) && !keypressed(KEY_EXE)) {
        pollevent(); sleep_ms(20);
    }
}

void FileManager::show_text_preview(const FileItem& item) {
    dclear(C_WHITE);
    drect(0, 0, SCREEN_W, 40, C_RGB(5, 5, 5));
    dtext_opt(SCREEN_W/2, 20, C_WHITE, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Text View", -1);
    dtext(10, 60, C_BLACK, "Sample content for");
    dtext(10, 80, C_BLACK, item.name.c_str());
    dupdate();
    while(!keypressed(KEY_EXIT) && !keypressed(KEY_EXE)) {
        pollevent(); sleep_ms(20);
    }
}

void FileManager::run() {
    std::vector<cinput::ListItem> lv_items;
    for(const auto& it : items) {
        cinput::ListItem li;
        li.text = it.name + (it.is_dir ? "/" : "");
        li.arrow = it.is_dir;
        lv_items.push_back(li);
    }

    cinput::ListView lv({0, 45, SCREEN_W, SCREEN_H - 45}, lv_items, 50, "dark");

    while(true) {
        lv.draw();
        draw_header();
        dupdate();
        cleareventflips();

        if (keypressed(KEY_EXIT)) return;

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
                    if (item.name == "..") { /* navigate up */ }
                    else { current_path += item.name + "\\"; refresh(); /* Update LV items here in real app */ }
                } else {
                    show_preview(item);
                }
            }
        }
        sleep_ms(20);
    }
}

} // namespace fm
