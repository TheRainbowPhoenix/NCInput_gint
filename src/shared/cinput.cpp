#include "cinput.hpp"
#include <gint/rtc.h>
#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/clock.h>
#include <cmath>
#include <cctype>
#include <algorithm>

namespace cinput {

// Helper to avoid ambiguity with gint's drect_border
static void cinput_drect_border_helper(int x1, int y1, int x2, int y2, int fill_color, int width, int border_color) {
    ::drect_border(x1, y1, x2, y2, fill_color, width, border_color);
}

std::map<std::string, Theme> THEMES = {
    {"light", {
        C_WHITE,
        C_WHITE,
        C_WHITE,
        C_RGB(28, 29, 28), // key_spec
        C_DARK,            // key_out
        C_RGB(4, 4, 4),    // txt
        C_RGB(8, 8, 8),    // txt_dim
        C_RGB(1, 11, 26),  // accent
        C_WHITE,           // txt_acc
        C_RGB(28, 29, 28), // hl
        C_WHITE            // check
    }},
    {"dark", {
        C_RGB(7, 7, 8),   // modal_bg
        C_RGB(7, 7, 8),   // kbd_bg
        C_RGB(7, 7, 8),   // key_bg
        C_RGB(11, 11, 12),// key_spec
        C_RGB(12, 19, 31),// key_out
        C_WHITE,          // txt
        C_RGB(8, 8, 8),   // txt_dim
        C_RGB(12, 19, 31),// accent
        C_WHITE,          // txt_acc
        C_RGB(11, 11, 12),// hl
        C_WHITE           // check
    }},
    {"grey", {
        C_LIGHT,
        C_LIGHT,
        C_WHITE,
        0xCE59,           // key_spec
        C_BLACK,          // key_out
        C_BLACK,          // txt
        C_RGB(8, 8, 8),   // txt_dim
        C_BLACK,          // accent
        C_WHITE,          // txt_acc
        0xCE59,           // hl
        C_WHITE           // check
    }}
};

const Theme& get_theme(const std::string& name) {
    auto it = THEMES.find(name);
    if (it != THEMES.end()) {
        return it->second;
    }
    auto light_it = THEMES.find("light");
    return light_it->second;
}

// =============================================================================
// REUSABLE LIST VIEW
// =============================================================================

ListView::ListView(Rect rect, const std::vector<ListItem>& items, int row_h, const std::string& theme, int headers_h)
    : m_rect(rect), m_items(items), m_base_row_h(row_h), m_theme(get_theme(theme))
{
    m_headers_h = (headers_h != -1) ? headers_h : row_h;
    m_drag_threshold = m_base_row_h;
    m_long_press_delay_ticks = 64; // ~500ms at 128Hz

    recalc_layout();
    select_next(0, 1);
}

void ListView::recalc_layout() {
    int total_h = 0;
    for (auto& it : m_items) {
        int h = (it.height != 0) ? it.height : (it.type == "section" ? m_headers_h : m_base_row_h);
        it._h = h;
        it._y = total_h;
        total_h += h;
    }
    m_total_h = total_h;
    m_max_scroll = (m_total_h > m_rect.h) ? (m_total_h - m_rect.h) : 0;
}

void ListView::select_next(int start_idx, int step) {
    int count = (int)m_items.size();
    if (count == 0) {
        m_selected_index = -1;
        return;
    }

    int idx = start_idx;
    for (int i = 0; i < count; ++i) {
        if (idx >= 0 && idx < count) {
            if (m_items[idx].type != "section") {
                m_selected_index = idx;
                ensure_visible();
                return;
            }
        } else {
            break;
        }
        idx += step;
    }
}

void ListView::ensure_visible() {
    if (m_selected_index < 0 || m_selected_index >= (int)m_items.size()) return;

    const auto& it = m_items[m_selected_index];
    int item_top = it._y;
    int item_bot = item_top + it._h;

    int view_top = m_scroll_y;
    int view_bot = m_scroll_y + m_rect.h;

    if (item_top < view_top) {
        m_scroll_y = item_top;
    } else if (item_bot > view_bot) {
        m_scroll_y = item_bot - m_rect.h;
    }

    clamp_scroll();
}

void ListView::clamp_scroll() {
    m_max_scroll = (m_total_h > m_rect.h) ? (m_total_h - m_rect.h) : 0;
    if (m_scroll_y > m_max_scroll) m_scroll_y = m_max_scroll;
    if (m_scroll_y < 0) m_scroll_y = 0;
}

bool ListView::update(const std::vector<key_event_t>& events, Action& out_action) {
    uint32_t now = rtc_ticks();

    const key_event_t* touch_up = nullptr;
    const key_event_t* touch_down = nullptr;
    const key_event_t* current_touch = nullptr;

    for (const auto& e : events) {
        if (e.type == KEYEV_TOUCH_DOWN) {
            touch_down = &e;
            current_touch = &e;
        } else if (e.type == KEYEV_TOUCH_UP) {
            touch_up = &e;
        } else if (e.type == KEYEV_TOUCH_DRAG) {
            current_touch = &e;
        }
    }

    if (!current_touch && touch_down) current_touch = touch_down;

    // 1. Start Touch
    if (touch_down && !m_is_dragging && m_touch_start_ticks == 0) {
        if (touch_down->x >= m_rect.x && touch_down->x < m_rect.x + m_rect.w &&
            touch_down->y >= m_rect.y && touch_down->y < m_rect.y + m_rect.h) {

            m_touch_start_y = touch_down->y;
            m_touch_start_ticks = now;

            int local_y = touch_down->y - m_rect.y + m_scroll_y;
            m_touch_initial_item_idx = get_index_at(local_y);

            if (m_touch_initial_item_idx != -1) {
                m_touch_start_idx = m_touch_initial_item_idx;
                if (m_items[m_touch_initial_item_idx].type != "section") {
                    m_selected_index = m_touch_initial_item_idx;
                    ensure_visible();
                }
            } else {
                m_touch_start_idx = m_selected_index;
            }

            m_is_dragging = false;
            m_long_press_triggered = false;
        }
    }

    // 2. Touch Move / Drag
    if (m_touch_start_ticks != 0) {
        const key_event_t* last_pos = current_touch ? current_touch : touch_down;
        if (last_pos) {
            int dy = last_pos->y - m_touch_start_y;

            if (!m_is_dragging) {
                if (std::abs(dy) > m_base_row_h) {
                    m_is_dragging = true;
                    m_long_press_triggered = true;
                }
            }

            if (m_is_dragging) {
                if (m_touch_start_idx >= 0 && m_touch_start_idx < (int)m_items.size()) {
                    int start_item_y = m_items[m_touch_start_idx]._y;
                    int target_y = start_item_y - dy;

                    int found_idx = -1;
                    for (int i = 0; i < (int)m_items.size(); ++i) {
                        if (m_items[i]._y <= target_y && target_y < m_items[i]._y + m_items[i]._h) {
                            found_idx = i;
                            break;
                        }
                    }

                    if (found_idx == -1) {
                        if (target_y < 0) found_idx = 0;
                        else found_idx = (int)m_items.size() - 1;
                    }

                    int final_idx = found_idx;
                    if (m_items[found_idx].type == "section") {
                        if (m_selected_index < found_idx) final_idx = found_idx - 1;
                        else final_idx = found_idx + 1;
                    }

                    if (final_idx >= 0 && final_idx < (int)m_items.size() && m_items[final_idx].type != "section") {
                        m_selected_index = final_idx;
                        ensure_visible();
                    }
                }
            }
        }
    }

    // 3. Touch Release
    if (touch_up) {
        if (m_touch_start_ticks != 0) {
            bool result = false;
            if (!m_is_dragging && !m_long_press_triggered) {
                int local_y = touch_up->y - m_rect.y + m_scroll_y;
                int release_idx = get_index_at(local_y);

                if (release_idx == m_touch_initial_item_idx && release_idx >= 0) {
                    if (m_items[release_idx].type != "section") {
                        m_selected_index = release_idx;
                        ensure_visible();
                        out_action = {"click", release_idx, m_items[release_idx]};
                        result = true;
                    }
                }
            }
            m_touch_start_ticks = 0;
            m_is_dragging = false;
            if (result) return true;
        }
    }

    // 4. Long Press
    if (m_touch_start_ticks != 0 && !m_is_dragging && !m_long_press_triggered) {
        if (now - m_touch_start_ticks > m_long_press_delay_ticks) {
            m_long_press_triggered = true;
            if (m_touch_initial_item_idx >= 0 && m_touch_initial_item_idx < (int)m_items.size()) {
                const auto& it = m_items[m_touch_initial_item_idx];
                if (it.type != "section") {
                    m_selected_index = m_touch_initial_item_idx;
                    out_action = {"long", m_touch_initial_item_idx, it};
                    return true;
                }
            }
        }
    }

    // 5. Keys
    for (const auto& e : events) {
        if (e.type == KEYEV_DOWN || (e.type == KEYEV_HOLD && (e.key == KEY_UP || e.key == KEY_DOWN))) {
            if (e.key == KEY_UP) {
                select_next(m_selected_index - 1, -1);
            } else if (e.key == KEY_DOWN) {
                select_next(m_selected_index + 1, 1);
            } else if (e.key == KEY_EXE) {
                if (m_selected_index >= 0) {
                    out_action = {"click", m_selected_index, m_items[m_selected_index]};
                    return true;
                }
            }
        }
    }

    return false;
}

int ListView::get_index_at(int y) {
    for (int i = 0; i < (int)m_items.size(); ++i) {
        if (m_items[i]._y <= y && y < m_items[i]._y + m_items[i]._h) {
            return i;
        }
    }
    return -1;
}

void ListView::draw_item(int x, int y, const ListItem& item, bool is_selected) {
    int h = item._h;
    if (item.type == "section") {
        drect(x, y, x + m_rect.w, y + h, m_theme.key_spec);
        cinput_drect_border_helper(x, y, x + m_rect.w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);
        dtext_opt(x + 10, y + h / 2, m_theme.txt_dim, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, item.text.c_str(), -1);
    } else {
        color_t bg = is_selected ? m_theme.hl : m_theme.modal_bg;
        drect(x, y, x + m_rect.w, y + h, bg);
        cinput_drect_border_helper(x, y, x + m_rect.w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);

        int x_off = 20;
        if (item.checked) {
            draw_check(x + 10, y + (h - 20) / 2, m_theme);
            x_off = 40;
        }

        dtext_opt(x + x_off, y + h / 2, m_theme.txt, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, item.text.c_str(), -1);

        if (item.arrow) {
            int ar_x = x + m_rect.w - 15;
            int ar_y = y + h / 2;
            color_t c = m_theme.txt_dim;
            dline(ar_x - 4, ar_y - 4, ar_x, ar_y, c);
            dline(ar_x - 4, ar_y + 4, ar_x, ar_y, c);
        }
    }
}

void ListView::draw() {
    drect(m_rect.x, m_rect.y, m_rect.x + m_rect.w, m_rect.y + m_rect.h, m_theme.modal_bg);

    int start_y = m_scroll_y;
    int end_y = m_scroll_y + m_rect.h;

    int start_idx = 0;
    for (int i = 0; i < (int)m_items.size(); ++i) {
        if (m_items[i]._y + m_items[i]._h > start_y) {
            start_idx = i;
            break;
        }
    }

    for (int i = start_idx; i < (int)m_items.size(); ++i) {
        const auto& it = m_items[i];
        if (it._y >= end_y) break;

        int item_y = m_rect.y + it._y - m_scroll_y;
        draw_item(m_rect.x, item_y, it, (i == m_selected_index));
    }

    if (m_max_scroll > 0) {
        int sb_w = 4;
        float ratio = (float)m_rect.h / (float)m_total_h;
        int thumb_h = (int)(m_rect.h * ratio);
        if (thumb_h < 20) thumb_h = 20;

        float scroll_ratio = (float)m_scroll_y / (float)m_max_scroll;
        int thumb_y = m_rect.y + (int)(scroll_ratio * (m_rect.h - thumb_h));

        int sb_x = m_rect.x + m_rect.w - sb_w - 2;
        drect(sb_x, thumb_y, sb_x + sb_w, thumb_y + thumb_h, m_theme.accent);
    }
}

void ListView::draw_check(int x, int y, const Theme& t) {
    drect(x, y, x + 20, y + 20, t.accent);
    color_t c = t.check;
    dline(x + 4, y + 10, x + 8, y + 14, c);
    dline(x + 8, y + 14, x + 15, y + 5, c);
}

// =============================================================================
// KEYBOARD WIDGET
// =============================================================================

static const std::map<std::string, std::vector<std::vector<std::string>>> LAYOUTS = {
    {"qwerty", {{"1","2","3","4","5","6","7","8","9","0"}, {"q","w","e","r","t","y","u","i","o","p"}, {"a","s","d","f","g","h","j","k","l",":"}, {"z","x","c","v","b","n","m",",",".","_"}}},
    {"azerty", {{"1","2","3","4","5","6","7","8","9","0"}, {"a","z","e","r","t","y","u","i","o","p"}, {"q","s","d","f","g","h","j","k","l","m"}, {"w","x","c","v","b","n",",",".","_",":"}}},
    {"qwertz", {{"1","2","3","4","5","6","7","8","9","0"}, {"q","w","e","r","t","z","u","i","o","p"}, {"a","s","d","f","g","h","j","k","l",":"}, {"y","x","c","v","b","n","m",",",".","_"}}},
    {"abc",    {{"1","2","3","4","5","6","7","8","9","0"}, {"a","b","c","d","e","f","g","h","i","j"}, {"k","l","m","n","o","p","q","r","s","t"}, {"u","v","w","x","y","z",",",".","_",":"}}}
};

static const std::vector<std::vector<std::string>> LAYOUT_SYM = {
    {"1","2","3","4","5","6","7","8","9","0"},
    {"@","#","$","_","&","-","+","(",")","/"},
    {"=","\\","<","*", "\"", "'", ":", ";", "!", "?"},
    {"{","}", "[", "]", "^", "~", "`", "|", "<", ">"}
};

Keyboard::Keyboard(int default_tab, bool enable_tabs, NumpadOpts numpad_opts, const std::string& theme, const std::string& layout)
    : m_current_tab(default_tab), m_enable_tabs(enable_tabs), m_numpad_opts(numpad_opts), m_theme(get_theme(theme))
{
    m_y = SCREEN_H - KBD_H;
    m_tabs = {"ABC", "Sym", "Math"};
    auto it = LAYOUTS.find(layout);
    if (it != LAYOUTS.end()) {
        m_layout_alpha = it->second;
    } else {
        auto qwerty_it = LAYOUTS.find("qwerty");
        m_layout_alpha = qwerty_it->second;
    }
    if (layout != "qwerty") {
        m_tabs[0] = (layout.length() <= 3) ? layout : "Txt";
        for (auto &c : m_tabs[0]) c = (char)std::toupper(c);
    }
}

void Keyboard::draw_key(int x, int y, int w, int h, const std::string& label, bool is_special, bool is_pressed, bool is_accent) {
    color_t bg;
    if (is_pressed) bg = m_theme.hl;
    else if (is_accent) bg = m_theme.accent;
    else if (is_special) bg = m_theme.key_spec;
    else bg = m_theme.key_bg;

    color_t txt_col = is_accent ? m_theme.txt_acc : m_theme.txt;
    color_t border_col = m_theme.key_spec;

    drect(x + 1, y + 1, x + w - 1, y + h - 1, bg);
    cinput_drect_border_helper(x, y, x + w, y + h, (int)C_NONE, 1, (int)border_col);
    dtext_opt(x + w/2, y + h/2, txt_col, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, label.c_str(), -1);
}

void Keyboard::draw_tabs() {
    int tab_w = SCREEN_W / 3;
    color_t border_col = m_theme.key_spec;
    for (int i = 0; i < (int)m_tabs.size(); ++i) {
        int tx = i * tab_w;
        bool is_active = (i == m_current_tab);
        color_t bg = is_active ? m_theme.kbd_bg : m_theme.key_spec;
        drect(tx, m_y, tx + tab_w, m_y + TAB_H, bg);
        cinput_drect_border_helper(tx, m_y, tx + tab_w, m_y + TAB_H, (int)C_NONE, 1, (int)border_col);
        if (is_active) {
            drect(tx + 1, m_y + TAB_H - 1, tx + tab_w - 1, m_y + TAB_H + 1, m_theme.kbd_bg);
        }
        dtext_opt(tx + tab_w/2, m_y + TAB_H/2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, m_tabs[i].c_str(), -1);
    }
}

void Keyboard::draw_grid() {
    const auto& layout = (m_current_tab == 1) ? LAYOUT_SYM : m_layout_alpha;
    int grid_y = m_y + TAB_H;
    int row_h = 45;
    for (int r = 0; r < (int)layout.size(); ++r) {
        int count = (int)layout[r].size();
        int kw = SCREEN_W / count;
        for (int c = 0; c < count; ++c) {
            int kx = c * kw;
            int ky = grid_y + r * row_h;
            std::string label = layout[r][c];
            if (m_current_tab == 0 && m_shift) {
                for (auto &ch : label) ch = (char)std::toupper(ch);
            }
            bool is_pressed = (m_last_key == label);
            draw_key(kx, ky, kw, row_h, label, false, is_pressed);
        }
    }

    int bot_y = grid_y + 4 * row_h;
    int bot_h = row_h;
    draw_key(0, bot_y, 50, bot_h, "CAPS", true, m_shift, false);
    draw_key(50, bot_y, 50, bot_h, "<-", true, m_last_key == "BACKSPACE", false);
    draw_key(100, bot_y, 160, bot_h, "Space", false, m_last_key == " ", false);
    draw_key(260, bot_y, 60, bot_h, "EXE", false, m_last_key == "ENTER", true);
}

std::string Keyboard::update_grid(int x, int y, int type) {
    int grid_y = m_y + TAB_H;
    int row_h = 45;
    int row_idx = (y - grid_y) / row_h;
    if (row_idx >= 0 && row_idx < 4) {
        const auto& layout = (m_current_tab == 1) ? LAYOUT_SYM : m_layout_alpha;
        if (row_idx >= (int)layout.size()) return "";
        const auto& row_chars = layout[row_idx];
        int kw = SCREEN_W / (int)row_chars.size();
        int col_idx = x / kw;
        if (col_idx < 0) col_idx = 0;
        if (col_idx >= (int)row_chars.size()) col_idx = (int)row_chars.size() - 1;
        std::string char_val = row_chars[col_idx];
        if (m_current_tab == 0 && m_shift) {
            for (auto &ch : char_val) ch = (char)std::toupper(ch);
        }
        if (type == KEYEV_TOUCH_DOWN) m_last_key = char_val;
        return char_val;
    } else if (row_idx == 4) {
        std::string cmd = "";
        if (x < 50) {
            if (type == KEYEV_TOUCH_DOWN) m_shift = !m_shift;
        } else if (x < 100) cmd = "BACKSPACE";
        else if (x < 260) cmd = " ";
        else cmd = "ENTER";
        if (type == KEYEV_TOUCH_DOWN) m_last_key = cmd;
        return cmd;
    }
    return "";
}

std::vector<KeyRect> Keyboard::get_math_rects() {
    std::vector<KeyRect> keys;
    int start_y = m_y + TAB_H;
    int total_h = KBD_H - TAB_H;
    int row_h = total_h / 4;
    int side_w = 50;
    int center_w = SCREEN_W - (side_w * 2);
    int numpad_w = center_w / 3;

    std::vector<std::string> l_chars = {"+", "-", "*", "/"};
    for (int i = 0; i < 4; ++i) {
        keys.push_back({0, start_y + i*row_h, side_w, row_h, l_chars[i], l_chars[i], true, false});
    }

    struct RChar { std::string disp; std::string val; bool spec; bool acc; };
    std::vector<RChar> r_chars = {{"%", "%", true, false}, {" ", " ", true, false}, {"<-", "BACKSPACE", true, false}, {"EXE", "ENTER", false, true}};
    for (int i = 0; i < 4; ++i) {
        keys.push_back({SCREEN_W - side_w, start_y + i*row_h, side_w, row_h, r_chars[i].disp, r_chars[i].val, r_chars[i].spec, r_chars[i].acc});
    }

    std::vector<std::vector<std::string>> nums = {{"1","2","3"}, {"4","5","6"}, {"7","8","9"}};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            keys.push_back({side_w + c*numpad_w, start_y + r*row_h, numpad_w, row_h, nums[r][c], nums[r][c], false, false});
        }
    }

    int y_bot = start_y + 3*row_h;
    int unit_w = center_w / 6;
    std::vector<std::string> bot_row = {",", "#", "0", "=", "."};
    std::vector<int> widths = {1, 1, 2, 1, 1};
    int cur_x = side_w;
    for (int i = 0; i < 5; ++i) {
        int w = widths[i] * unit_w;
        if (i == 4) w = (side_w + center_w) - cur_x;
        keys.push_back({cur_x, y_bot, w, row_h, bot_row[i], bot_row[i], false, false});
        cur_x += w;
    }
    return keys;
}

std::vector<KeyRect> Keyboard::get_numpad_rects() {
    std::vector<KeyRect> keys;
    int start_y = m_y;
    int total_h = KBD_H;
    int row_h = total_h / 4;
    int action_w = 80;
    int digit_w = (SCREEN_W - action_w) / 3;

    keys.push_back({SCREEN_W - action_w, start_y, action_w, row_h, "<-", "BACKSPACE", true, false});
    keys.push_back({SCREEN_W - action_w, start_y + row_h, action_w, row_h*3, "EXE", "ENTER", false, true});

    std::vector<std::vector<std::string>> nums = {{"1","2","3"}, {"4","5","6"}, {"7","8","9"}};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            keys.push_back({c*digit_w, start_y + r*row_h, digit_w, row_h, nums[r][c], nums[r][c], false, false});
        }
    }

    int y_bot = start_y + 3*row_h;
    std::vector<std::string> bot_keys;
    if (m_numpad_opts.is_neg) bot_keys.push_back("-");
    bot_keys.push_back("0");
    if (m_numpad_opts.is_float) bot_keys.push_back(".");

    if (!bot_keys.empty()) {
        int bw = (SCREEN_W - action_w) / (int)bot_keys.size();
        int cur_x = 0;
        for (int i = 0; i < (int)bot_keys.size(); ++i) {
            int w = bw;
            if (i == (int)bot_keys.size() - 1) w = (SCREEN_W - action_w) - cur_x;
            keys.push_back({cur_x, y_bot, w, row_h, bot_keys[i], bot_keys[i], false, false});
            cur_x += w;
        }
    }
    return keys;
}

void Keyboard::draw_keys_from_rects(const std::vector<KeyRect>& rects) {
    for (const auto& r : rects) {
        bool is_pressed = (m_last_key == r.val);
        draw_key(r.x, r.y, r.w, r.h, r.label, r.is_spec, is_pressed, r.is_acc);
    }
}

std::string Keyboard::update_keys_from_rects(const std::vector<KeyRect>& rects, int x, int y, int type) {
    for (const auto& r : rects) {
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            if (type == KEYEV_TOUCH_DOWN) m_last_key = r.val;
            return r.val;
        }
    }
    return "";
}

void Keyboard::draw() {
    if (!m_visible) return;
    drect(0, m_y, SCREEN_W, SCREEN_H, m_theme.kbd_bg);
    dhline(m_y, m_theme.key_spec);
    if (m_enable_tabs) {
        draw_tabs();
        if (m_current_tab == 2) draw_keys_from_rects(get_math_rects());
        else draw_grid();
    } else {
        draw_keys_from_rects(get_numpad_rects());
    }
}

std::string Keyboard::update(const key_event_t& ev) {
    if (ev.type == KEYEV_TOUCH_DOWN) m_last_key = "";
    if (!m_visible) return "";

    int x = ev.x, y = ev.y;
    if (y < m_y) return "";

    if (m_enable_tabs && y < m_y + TAB_H) {
        if (ev.type == KEYEV_TOUCH_DOWN) {
            int tab_w = SCREEN_W / 3;
            m_current_tab = x / tab_w;
            if (m_current_tab > 2) m_current_tab = 2;
        }
        return "";
    }

    if (!m_enable_tabs) return update_keys_from_rects(get_numpad_rects(), x, y, ev.type);
    if (m_current_tab == 2) return update_keys_from_rects(get_math_rects(), x, y, ev.type);
    return update_grid(x, y, ev.type);
}

// =============================================================================
// LIST PICKER WIDGET
// =============================================================================

ListPicker::ListPicker(const std::vector<std::string>& options, const std::string& prompt, const std::string& theme, bool multi, int touch_mode)
    : m_options(options), m_prompt(prompt), m_theme(get_theme(theme)), m_theme_name(theme), m_multi(multi), m_touch_mode(touch_mode)
{
    m_header_h = PICK_HEADER_H;
    m_footer_h = PICK_FOOTER_H;
    m_view_h = SCREEN_H - m_header_h - m_footer_h;
    m_btn_w = 60;

    std::vector<ListItem> lv_items;
    for (int i = 0; i < (int)options.size(); ++i) {
        ListItem it;
        it.text = options[i];
        it.idx = i;
        if (m_multi) it.checked = false;
        lv_items.push_back(it);
    }

    m_list_view = std::unique_ptr<ListView>(new ListView({0, m_header_h, SCREEN_W, m_view_h}, lv_items, PICK_ITEM_H, m_theme_name));
    if (!m_multi && !lv_items.empty()) {
        m_list_view->m_selected_index = 0;
    }
}

ListPicker::~ListPicker() {}

void ListPicker::draw_nav_btn(int x, int w, int h, const std::string& type, bool is_pressed) {
    int y = SCREEN_H - h;
    color_t bg = is_pressed ? m_theme.hl : m_theme.key_spec;

    drect(x, y, x + w, SCREEN_H, bg);
    cinput_drect_border_helper(x, y, x + w, SCREEN_H, (int)C_NONE, 1, (int)m_theme.key_spec);

    int cx = x + w/2, cy = y + h/2;
    color_t col = m_theme.txt;

    if (type == "UP") {
        int px[] = {cx, cx-5, cx+5};
        int py[] = {cy-5, cy+5, cy+5};
        dpoly(px, py, 3, col, (int)C_NONE);
    } else if (type == "DOWN") {
        int px[] = {cx, cx-5, cx+5};
        int py[] = {cy+5, cy-5, cy-5};
        dpoly(px, py, 3, col, (int)C_NONE);
    }
}

void ListPicker::draw_close_icon(int x, int y, int sz, color_t col) {
    dline(x, y, x+sz, y+sz, col);
    dline(x, y+sz, x+sz, y, col);
    dline(x+1, y, x+sz+1, y+sz, col);
    dline(x+1, y+sz, x+sz+1, y, col);
}

void ListPicker::draw() {
    dclear(m_theme.modal_bg);

    if (m_multi) {
        for (auto& it : m_list_view->m_items) {
            it.checked = false;
            for (int idx : m_selected_indices) {
                if (it.idx == idx) {
                    it.checked = true;
                    break;
                }
            }
        }
    }

    m_list_view->draw();

    // Header
    drect(0, 0, SCREEN_W, m_header_h, m_theme.accent);
    dtext_opt(SCREEN_W/2, m_header_h/2, m_theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, m_prompt.c_str(), -1);
    draw_close_icon(15, 15, 10, m_theme.txt_acc);

    // Footer
    int fy = SCREEN_H - m_footer_h;
    drect(0, fy, SCREEN_W, SCREEN_H, m_theme.key_spec);
    dhline(fy, m_theme.key_spec);

    draw_nav_btn(0, m_btn_w, m_footer_h, "UP", m_last_action == "PAGE_UP");
    draw_nav_btn(SCREEN_W - m_btn_w, m_btn_w, m_footer_h, "DOWN", m_last_action == "PAGE_DOWN");

    bool ok_pressed = (m_last_action == "OK");
    color_t ok_bg = ok_pressed ? m_theme.hl : m_theme.key_spec;
    int ok_rect_x = m_btn_w;
    int ok_rect_w = SCREEN_W - 2 * m_btn_w;
    drect(ok_rect_x, fy, ok_rect_x + ok_rect_w, SCREEN_H, ok_bg);
    cinput_drect_border_helper(ok_rect_x, fy, ok_rect_x + ok_rect_w, SCREEN_H, (int)C_NONE, 1, (int)m_theme.key_spec);

    const char* label = m_multi ? "OK" : "Select";
    dtext_opt(SCREEN_W/2, fy + m_footer_h/2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, label, -1);
}

PickResult ListPicker::run() {
    clearevents();
    cleareventflips();

    bool touch_latched = false;

    while (true) {
        draw();
        dupdate();
        cleareventflips();

        key_event_t ev = pollevent();
        std::vector<key_event_t> events;
        while (ev.type != KEYEV_NONE) {
            events.push_back(ev);
            ev = pollevent();
        }

        if (keypressed(KEY_EXIT) || keypressed(KEY_DEL)) {
            return {false, {}};
        }

        bool key_pg_up = keypressed(KEY_LEFT);
        bool key_pg_dn = keypressed(KEY_RIGHT);

        std::vector<key_event_t> lv_events;
        const key_event_t* footer_touch = nullptr;
        const key_event_t* header_touch = nullptr;

        for (const auto& e : events) {
            if (e.type == KEYEV_TOUCH_DOWN || e.type == KEYEV_TOUCH_UP || e.type == KEYEV_TOUCH_DRAG) {
                if (e.y < m_header_h) {
                    header_touch = &e;
                } else if (e.y >= SCREEN_H - m_footer_h) {
                    footer_touch = &e;
                } else {
                    lv_events.push_back(e);
                }
            } else {
                lv_events.push_back(e);
            }
        }

        ListView::Action action;
        if (m_list_view->update(lv_events, action)) {
            if (action.type == "click") {
                if (m_multi) {
                    int real_idx = action.item.idx;
                    bool found = false;
                    for (auto it = m_selected_indices.begin(); it != m_selected_indices.end(); ++it) {
                        if (*it == real_idx) {
                            m_selected_indices.erase(it);
                            found = true;
                            break;
                        }
                    }
                    if (!found) m_selected_indices.push_back(real_idx);
                } else {
                    return { true, { m_options[action.item.idx] } };
                }
            }
        }

        if (key_pg_up) {
            m_list_view->m_scroll_y -= m_list_view->m_rect.h;
            m_list_view->clamp_scroll();
        }
        if (key_pg_dn) {
            m_list_view->m_scroll_y += m_list_view->m_rect.h;
            m_list_view->clamp_scroll();
        }

        bool ignore_action = (m_touch_mode == KEYEV_TOUCH_DOWN && touch_latched);

        if (header_touch && header_touch->type == m_touch_mode) {
            if (!ignore_action && header_touch->x < 40) {
                return {false, {}};
            }
        }

        if (footer_touch) {
            if (footer_touch->type == m_touch_mode && !ignore_action) {
                if (footer_touch->x < m_btn_w) m_last_action = "PAGE_UP";
                else if (footer_touch->x > SCREEN_W - m_btn_w) m_last_action = "PAGE_DOWN";
                else m_last_action = "OK";

                if (m_touch_mode == KEYEV_TOUCH_DOWN) touch_latched = true;
            }
        }

        for (const auto& e : events) {
            if (e.type == KEYEV_TOUCH_UP) touch_latched = false;
        }

        if (!m_last_action.empty()) {
            if (m_last_action == "PAGE_UP") {
                m_list_view->m_scroll_y -= m_list_view->m_rect.h;
                m_list_view->clamp_scroll();
            } else if (m_last_action == "PAGE_DOWN") {
                m_list_view->m_scroll_y += m_list_view->m_rect.h;
                m_list_view->clamp_scroll();
            } else if (m_last_action == "OK") {
                if (m_multi) {
                    std::vector<std::string> res;
                    std::sort(m_selected_indices.begin(), m_selected_indices.end());
                    for (int idx : m_selected_indices) res.push_back(m_options[idx]);
                    return {true, res};
                } else {
                    if (m_list_view->m_selected_index >= 0) {
                        return { true, { m_options[m_list_view->m_items[m_list_view->m_selected_index].idx] } };
                    }
                }
            }
            m_last_action = "";
        }

        sleep_ms(10);
    }
}

// =============================================================================
// CONFIRMATION DIALOG
// =============================================================================

ConfirmationDialog::ConfirmationDialog(const std::string& title, const std::string& body, const std::string& ok_text, const std::string& cancel_text, const std::string& theme, int touch_mode)
    : m_title(title), m_body(body), m_ok_text(ok_text), m_cancel_text(cancel_text), m_theme(get_theme(theme)), m_touch_mode(touch_mode)
{
    m_header_h = 40;
    m_footer_h = 45;
    m_btn_w = SCREEN_W / 2;
}

void ConfirmationDialog::draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed) {
    color_t bg = pressed ? m_theme.hl : m_theme.key_spec;
    drect(x, y, x + w, y + h, bg);
    cinput_drect_border_helper(x, y, x + w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);
    dtext_opt(x + w/2, y + h/2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, text.c_str(), -1);
}

void ConfirmationDialog::draw(bool btn_ok_pressed, bool btn_cn_pressed) {
    dclear(m_theme.modal_bg);

    // Header
    drect(0, 0, SCREEN_W, m_header_h, m_theme.accent);
    dtext_opt(SCREEN_W/2, m_header_h/2, m_theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, m_title.c_str(), -1);

    // Body
    int cy = (SCREEN_H - m_header_h - m_footer_h) / 2 + m_header_h;
    dtext_opt(SCREEN_W/2, cy, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, m_body.c_str(), -1);

    // Footer
    int fy = SCREEN_H - m_footer_h;
    draw_btn(0, fy, m_btn_w, m_footer_h, m_cancel_text, btn_cn_pressed);
    draw_btn(m_btn_w, fy, m_btn_w, m_footer_h, m_ok_text, btn_ok_pressed);
}

bool ConfirmationDialog::run() {
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
        std::vector<key_event_t> events;
        while (ev.type != KEYEV_NONE) {
            events.push_back(ev);
            ev = pollevent();
        }

        const key_event_t* touch = nullptr;
        for (const auto& e : events) {
            if (e.type == KEYEV_TOUCH_DOWN && !touch_latched) {
                touch_latched = true;
                touch = &e;
            } else if (e.type == KEYEV_TOUCH_UP) {
                touch_latched = false;
                touch = &e;
            }
        }

        if (touch) {
            int tx = touch->x, ty = touch->y;
            int fy = SCREEN_H - m_footer_h;

            bool is_cancel = (ty >= fy && tx < m_btn_w);
            bool is_ok = (ty >= fy && tx >= m_btn_w);

            if (touch->type == KEYEV_TOUCH_DOWN) {
                if (is_cancel) btn_cn_pressed = true;
                else if (is_ok) btn_ok_pressed = true;
            }

            if (touch->type == m_touch_mode) {
                if (is_cancel) return false;
                else if (is_ok) return true;
            }

            if (touch->type == KEYEV_TOUCH_UP) {
                btn_cn_pressed = false;
                btn_ok_pressed = false;
            }
        }

        sleep_ms(10);
    }
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

InputResult input(const std::string& prompt, const std::string& type, const std::string& theme_name, const std::string& layout, int touch_mode) {
    const Theme& t = get_theme(theme_name);
    clearevents();
    cleareventflips();

    int start_tab = 0;
    bool enable_tabs = true;
    NumpadOpts numpad_opts = {true, true};

    if (type.find("numeric_") != std::string::npos) {
        enable_tabs = false;
        numpad_opts.is_float = (type.find("int") == std::string::npos);
        numpad_opts.is_neg = (type.find("negative") != std::string::npos);
    } else if (type == "math") {
        start_tab = 2;
    }

    Keyboard kbd(start_tab, enable_tabs, numpad_opts, theme_name, layout);
    std::string text = "";
    bool touch_latched = false;

    while (true) {
        dclear(t.modal_bg);

        // Header
        drect(0, 0, SCREEN_W, PICK_HEADER_H, t.accent);
        dtext_opt(SCREEN_W/2, PICK_HEADER_H/2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, prompt.c_str(), -1);

        // Close Button
        dline(15, 15, 25, 25, t.txt_acc);
        dline(25, 15, 15, 25, t.txt_acc);
        dline(16, 15, 26, 25, t.txt_acc);
        dline(26, 15, 16, 25, t.txt_acc);

        // Input Box
        int box_y = 60, box_h = 40;
        cinput_drect_border_helper(10, box_y, SCREEN_W - 10, box_y + box_h, (int)t.hl, 2, (int)t.txt);
        std::string display_text = text + "_";
        dtext(15, box_y + 10, t.txt, display_text.c_str());

        kbd.draw();
        dupdate();
        cleareventflips();

        if (keypressed(KEY_EXIT)) return {false, ""};
        if (keypressed(KEY_EXE)) return {true, text};
        if (keypressed(KEY_DEL) && !text.empty()) text.pop_back();

        key_event_t ev = pollevent();
        std::vector<key_event_t> events;
        while (ev.type != KEYEV_NONE) {
            events.push_back(ev);
            ev = pollevent();
        }

        for (const auto& e : events) {
            if (e.type == KEYEV_TOUCH_UP) {
                touch_latched = false;
                kbd.m_last_key = "";
            }

            bool should_close = (e.type == touch_mode);
            if (touch_mode == KEYEV_TOUCH_DOWN && touch_latched) should_close = false;

            if (should_close && e.y < PICK_HEADER_H && e.x < 40) {
                return {false, ""};
            }

            if (e.type == KEYEV_TOUCH_DOWN && !touch_latched) {
                if (!(e.y < PICK_HEADER_H && e.x < 40)) {
                    touch_latched = true;
                    std::string res = kbd.update(e);
                    if (!res.empty()) {
                        if (res == "ENTER") {
                            return {true, text};
                        } else if (res == "BACKSPACE") {
                            if (!text.empty()) text.pop_back();
                        } else if (res.length() == 1) {
                            text += res;
                        }
                    }
                }
            }
        }

        sleep_ms(10);
    }
}

PickResult pick(const std::vector<std::string>& options, const std::string& prompt, const std::string& theme, bool multi, int touch_mode) {
    ListPicker picker(options, prompt, theme, multi, touch_mode);
    return picker.run();
}

bool ask(const std::string& title, const std::string& body, const std::string& ok_text, const std::string& cancel_text, const std::string& theme, int touch_mode) {
    ConfirmationDialog dlg(title, body, ok_text, cancel_text, theme, touch_mode);
    return dlg.run();
}

} // namespace cinput
