#include "cinput.hpp"
#include "mcs.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <gint/rtc.h>
#include <cstdio>
#include <algorithm>

namespace mcs_app {

class MCSListView : public cinput::ListView {
public:
    using cinput::ListView::ListView;

    void draw_item(int x, int y, const cinput::ListItem& item, bool is_selected) override {
        const auto& t = m_theme;
        int h = item._h;

        if (item.type == "section") {
            drect(x, y, x + m_rect.w, y + h, t.key_spec);
            drect_border(x, y, x + m_rect.w, y + h, (int)C_NONE, 1, (int)t.key_spec);
            dtext_opt(x + 10, y + h / 2, t.txt_dim, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, item.text.c_str(), -1);
        } else {
            color_t bg = is_selected ? t.hl : t.modal_bg;
            drect(x, y, x + m_rect.w, y + h, bg);
            drect_border(x, y, x + m_rect.w, y + h, (int)C_NONE, 1, (int)t.key_spec);

            color_t col = t.txt;
            color_t icon_col = is_selected ? t.txt : t.accent;

            if (item.type == "folder") {
                // Breeze style Folder Icon
                color_t inner_col = t.txt_dim;
                int px1[] = {x+8, x+16, x+20, x+20, x+8};
                int py1[] = {y+12, y+12, y+16, y+22, y+22};
                dpoly(px1, py1, 5, inner_col, (int)inner_col);

                int px2[] = {x+8, x+14, x+19, x+32, x+32, x+8};
                int py2[] = {y+21, y+21, y+16, y+16, y+35, y+35};
                dpoly(px2, py2, 6, icon_col, (int)icon_col);
            } else if (item.type == "var") {
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

            if (item.arrow) {
                int ar_x = x + m_rect.w - 15;
                int ar_y = y + h / 2;
                color_t c = t.txt_dim;
                dline(ar_x - 4, ar_y - 4, ar_x, ar_y, c);
                dline(ar_x - 4, ar_y + 4, ar_x, ar_y, c);
            }
        }
    }
};

class HexViewerDialog {
    mcs::Variable m_var;
    cinput::Theme m_theme;
    int m_header_h = 40;
    int m_footer_h = 45;
    int m_btn_w = SCREEN_W / 2;
    uint32_t m_max_bytes;

public:
    HexViewerDialog(const mcs::Variable& var, const std::string& theme)
        : m_var(var), m_theme(cinput::get_theme(theme)) {
        m_max_bytes = std::min((uint32_t)256, var.size);
    }

    void draw_btn(int x, int y, int w, int h, const char* text, bool pressed) {
        color_t bg = pressed ? m_theme.hl : m_theme.key_spec;
        drect(x, y, x + w, y + h, bg);
        drect_border(x, y, x + w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);
        dtext_opt(x + w / 2, y + h / 2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, text, -1);
    }

    void draw(bool btn_ok_pressed, bool btn_cn_pressed) {
        dclear(m_theme.modal_bg);
        drect(0, 0, SCREEN_W, m_header_h, m_theme.accent);
        dtext_opt(SCREEN_W / 2, m_header_h / 2, m_theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, m_var.name.c_str(), -1);

        char info[64];
        sprintf(info, "Type: %d | Size: %d bytes", m_var.type, m_var.size);
        dtext_opt(SCREEN_W / 2, m_header_h + 15, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, info, -1);

        int hex_y = m_header_h + 40;
        int hex_h = SCREEN_H - m_footer_h - hex_y - 5;
        drect(10, hex_y, SCREEN_W - 10, hex_y + hex_h, m_theme.key_bg);
        drect_border(10, hex_y, SCREEN_W - 10, hex_y + hex_h, (int)C_NONE, 1, (int)m_theme.key_spec);

        int ly = hex_y + 4;
        for (uint32_t offset = 0; offset < m_max_bytes; offset += 8) {
            if (ly > hex_y + hex_h - 12) break;
            char off_str[8]; sprintf(off_str, "%04X", offset);
            dtext(15, ly, m_theme.txt, off_str);

            for (int i = 0; i < 8 && offset + i < m_max_bytes; ++i) {
                uint8_t b = *(uint8_t*)(m_var.ptr + offset + i); // WARNING: Direct read
                char b_str[4]; sprintf(b_str, "%02X", b);
                color_t c = (b == 0) ? m_theme.txt_dim : m_theme.txt;
                dtext(55 + i * 18, ly, c, b_str);

                char ch = (b >= 32 && b < 127) ? (char)b : '.';
                char ch_str[2] = {ch, 0};
                dtext(210 + i * 11, ly, m_theme.txt, ch_str);
            }
            ly += 12;
        }

        if (m_var.size > m_max_bytes) {
            dtext_opt(SCREEN_W / 2, ly + 2, m_theme.txt_dim, (int)C_NONE, DTEXT_CENTER, DTEXT_TOP, "... (truncated) ...", -1);
        }

        int fy = SCREEN_H - m_footer_h;
        draw_btn(0, fy, m_btn_w, m_footer_h, "Cancel", btn_cn_pressed);
        draw_btn(m_btn_w, fy, m_btn_w, m_footer_h, "Dump to File", btn_ok_pressed);
    }

    bool run() {
        clearevents();
        cleareventflips();
        bool touch_latched = false;
        bool btn_ok_pressed = false;
        bool btn_cn_pressed = false;

        while (true) {
            draw(btn_ok_pressed, btn_cn_pressed);
            dupdate();
            cleareventflips();

            if (keypressed(KEY_EXIT) || keypressed(KEY_DEL)) return false;
            if (keypressed(KEY_EXE)) return true;

            key_event_t ev = pollevent();
            const key_event_t* touch = nullptr;
            if (ev.type == KEYEV_TOUCH_DOWN && !touch_latched) { touch_latched = true; touch = &ev; }
            else if (ev.type == KEYEV_TOUCH_UP) { touch_latched = false; touch = &ev; }

            if (touch) {
                int tx = touch->x, ty = touch->y;
                int fy = SCREEN_H - m_footer_h;
                bool is_cancel = (ty >= fy && tx < m_btn_w);
                bool is_ok = (ty >= fy && tx >= m_btn_w);

                if (touch->type == KEYEV_TOUCH_DOWN) {
                    if (is_cancel) btn_cn_pressed = true;
                    else if (is_ok) btn_ok_pressed = true;
                }
                if (touch->type == KEYEV_TOUCH_UP) {
                    if (is_cancel) return false;
                    if (is_ok) return true;
                    btn_cn_pressed = false; btn_ok_pressed = false;
                }
            }
            sleep_ms(10);
        }
    }
};

class MCSBrowser {
    std::string m_theme_name = "light";
    std::vector<mcs::Directory> m_folders;
    int m_current_folder_idx = -1;
    int m_main_scroll = 0;
    std::unique_ptr<MCSListView> m_list_view;

public:
    MCSBrowser() {
        m_folders = mcs::get_directories();
        rebuild_list();
    }

    void rebuild_list() {
        std::vector<cinput::ListItem> items;
        if (m_current_folder_idx == -1) {
            items.push_back({"Main Control System", 0, "section", 30});
            for (const auto& f : m_folders) {
                char buf[64];
                sprintf(buf, "%s (%d items)", f.name.c_str(), f.var_num);
                cinput::ListItem it;
                it.text = buf; it.type = "folder"; it.arrow = true; it.height = 50;
                items.push_back(it);
            }
        } else {
            const auto& folder = m_folders[m_current_folder_idx];
            items.push_back({folder.name, 0, "section", 30});
            auto vars = mcs::get_variables(folder);
            for (const auto& v : vars) {
                cinput::ListItem it;
                it.text = v.name; it.type = "var"; it.height = 50;
                items.push_back(it);
            }
        }
        m_list_view = std::make_unique<MCSListView>(cinput::Rect{0, 40, SCREEN_W, SCREEN_H - 40}, items, 50, m_theme_name, 30);
    }

    void dump_variable(const mcs::Variable& var) {
        char filename[64];
        sprintf(filename, "%s.bin", var.name.c_str());
        FILE* f = fopen(filename, "wb");
        if (f) {
            fwrite((void*)var.ptr, 1, var.size, f);
            fclose(f);
            cinput::ask("Success", "Saved as " + std::string(filename), "OK", "", m_theme_name);
        } else {
            cinput::ask("Error", "Could not save file", "OK", "", m_theme_name);
        }
    }

    void run() {
        bool running = true;
        const auto& t = cinput::get_theme(m_theme_name);
        clearevents();
        cleareventflips();

        while (running) {
            dclear(t.modal_bg);
            m_list_view->draw();
            drect(0, 0, SCREEN_W, 40, t.accent);
            if (m_current_folder_idx != -1) {
                dtext_opt(15, 20, t.txt_acc, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, "< Back", -1);
            } else {
                dtext_opt(SCREEN_W / 2, 20, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "MCS Browser", -1);
            }
            dupdate();
            cleareventflips();

            if (keypressed(KEY_EXIT) || keypressed(KEY_DEL)) {
                if (m_current_folder_idx != -1) {
                    m_current_folder_idx = -1; rebuild_list(); m_list_view->m_scroll_y = m_main_scroll;
                } else running = false;
            }

            key_event_t ev = pollevent();
            std::vector<key_event_t> events;
            while (ev.type != KEYEV_NONE) {
                if (ev.type == KEYEV_TOUCH_UP && ev.y < 40 && ev.x < 80) {
                    if (m_current_folder_idx != -1) {
                        m_current_folder_idx = -1; rebuild_list(); m_list_view->m_scroll_y = m_main_scroll;
                    }
                }
                events.push_back(ev);
                ev = pollevent();
            }

            cinput::ListView::Action action;
            if (m_list_view->update(events, action)) {
                if (action.type == "click") {
                    if (action.item.type == "folder") {
                        m_main_scroll = m_list_view->m_scroll_y;
                        m_current_folder_idx = action.index - 1; // -1 because of section header
                        rebuild_list();
                    } else if (action.item.type == "var") {
                        const auto& folder = m_folders[m_current_folder_idx];
                        auto vars = mcs::get_variables(folder);
                        auto v = vars[action.index - 1];
                        HexViewerDialog viewer(v, m_theme_name);
                        if (viewer.run()) dump_variable(v);
                        clearevents();
                    }
                }
            }
            sleep_ms(10);
        }
    }
};

} // namespace mcs_app

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif
    mcs_app::MCSBrowser app;
    app.run();
    return 0;
}
