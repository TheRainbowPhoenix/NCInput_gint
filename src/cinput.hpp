#ifndef CINPUT_HPP
#define CINPUT_HPP

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace cinput {

// =============================================================================
// CONSTANTS & CONFIG
// =============================================================================

// Layout Dimensions
const int KBD_H = 260;
const int TAB_H = 30;
const int PICK_HEADER_H = 40;
const int PICK_FOOTER_H = 45;
const int PICK_ITEM_H = 50;

// =============================================================================
// THEMES
// =============================================================================

struct Theme {
    color_t modal_bg;
    color_t kbd_bg;
    color_t key_bg;
    color_t key_spec;
    color_t key_out;
    color_t txt;
    color_t txt_dim;
    color_t accent;
    color_t txt_acc;
    color_t hl;
    color_t check;
};

extern const std::map<std::string, Theme> THEMES;

const Theme& get_theme(const std::string& name);

// =============================================================================
// REUSABLE LIST VIEW
// =============================================================================

struct ListItem {
    std::string text;
    std::string type = "item"; // "item" or "section"
    int height = 0;            // 0 means use default
    bool checked = false;
    bool arrow = false;
    int idx = -1;

    // Internal layout state
    int _h = 0;
    int _y = 0;
};

class ListView {
public:
    struct Rect { int x, y, w, h; };
    struct Action {
        std::string type; // "click", "long"
        int index;
        ListItem item;
    };

    ListView(Rect rect, const std::vector<ListItem>& items, int row_h = 40, const std::string& theme = "light", int headers_h = -1);

    void recalc_layout();
    void select_next(int start_idx, int step);
    void ensure_visible();
    void clamp_scroll();

    bool update(const std::vector<key_event_t>& events, Action& out_action);

    int get_index_at(int y);
    void draw_item(int x, int y, const ListItem& item, bool is_selected);
    void draw();
    void draw_check(int x, int y, const Theme& t);

    Rect m_rect;
    std::vector<ListItem> m_items;
    int m_base_row_h;
    int m_headers_h;
    const Theme& m_theme;

    int m_total_h = 0;
    int m_selected_index = -1;
    int m_scroll_y = 0;
    int m_max_scroll = 0;

    // Touch State
    bool m_is_dragging = false;
    int m_touch_start_y = 0;
    int m_touch_start_idx = 0;
    uint32_t m_touch_start_ticks = 0;
    int m_touch_initial_item_idx = -1;
    bool m_long_press_triggered = false;

    // Configuration
    int m_drag_threshold;
    uint32_t m_long_press_delay_ticks = 64; // ~500ms at 128Hz
};

// =============================================================================
// KEYBOARD WIDGET
// =============================================================================

struct NumpadOpts {
    bool is_float = true;
    bool is_neg = true;
};

struct KeyRect {
    int x, y, w, h;
    std::string label;
    std::string val;
    bool is_spec;
    bool is_acc;
};

class Keyboard {
public:
    Keyboard(int default_tab = 0, bool enable_tabs = true, NumpadOpts numpad_opts = {true, true}, const std::string& theme = "light", const std::string& layout = "qwerty");

    void draw_key(int x, int y, int w, int h, const std::string& label, bool is_special = false, bool is_pressed = false, bool is_accent = false);
    void draw_tabs();
    void draw_grid();
    std::string update_grid(int x, int y, int type);

    std::vector<KeyRect> get_math_rects();
    std::vector<KeyRect> get_numpad_rects();
    void draw_keys_from_rects(const std::vector<KeyRect>& rects);
    std::string update_keys_from_rects(const std::vector<KeyRect>& rects, int x, int y, int type);

    void draw();
    std::string update(const key_event_t& ev);

    int m_y;
    bool m_visible = true;
    int m_current_tab;
    bool m_enable_tabs;
    bool m_shift = false;
    std::vector<std::string> m_tabs;
    std::string m_last_key;
    NumpadOpts m_numpad_opts;
    const Theme& m_theme;
    std::vector<std::vector<std::string>> m_layout_alpha;
};

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

struct InputResult {
    bool success;
    std::string value;
};

struct PickResult {
    bool success;
    std::vector<std::string> values;
};

InputResult input(const std::string& prompt = "Input:", const std::string& type = "alpha_numeric", const std::string& theme = "light", const std::string& layout = "qwerty", int touch_mode = KEYEV_TOUCH_DOWN);
PickResult pick(const std::vector<std::string>& options, const std::string& prompt = "Select:", const std::string& theme = "light", bool multi = false, int touch_mode = KEYEV_TOUCH_DOWN);

// =============================================================================
// LIST PICKER WIDGET
// =============================================================================

class ListPicker {
public:
    ListPicker(const std::vector<std::string>& options, const std::string& prompt = "Select:", const std::string& theme = "light", bool multi = false, int touch_mode = KEYEV_TOUCH_DOWN);
    ~ListPicker();

    void draw_nav_btn(int x, int w, int h, const std::string& type, bool is_pressed);
    void draw_close_icon(int x, int y, int sz, color_t col);
    void draw();
    PickResult run();

    std::vector<std::string> m_options;
    std::string m_prompt;
    const Theme& m_theme;
    std::string m_theme_name;
    bool m_multi;
    int m_touch_mode;
    std::vector<int> m_selected_indices; // for multi
    int m_single_selection = 0;

    int m_header_h;
    int m_footer_h;
    int m_view_h;
    int m_btn_w;
    std::string m_last_action;

    std::unique_ptr<ListView> m_list_view;
};

// =============================================================================
// CONFIRMATION DIALOG
// =============================================================================

class ConfirmationDialog {
public:
    ConfirmationDialog(const std::string& title, const std::string& body, const std::string& ok_text = "OK", const std::string& cancel_text = "Cancel", const std::string& theme = "light", int touch_mode = KEYEV_TOUCH_DOWN);

    void draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed);
    void draw(bool btn_ok_pressed, bool btn_cn_pressed);
    bool run();

    std::string m_title;
    std::string m_body;
    std::string m_ok_text;
    std::string m_cancel_text;
    const Theme& m_theme;
    int m_touch_mode;
    int m_header_h;
    int m_footer_h;
    int m_btn_w;
};

bool ask(const std::string& title, const std::string& body, const std::string& ok_text = "OK", const std::string& cancel_text = "Cancel", const std::string& theme = "light", int touch_mode = KEYEV_TOUCH_DOWN);

} // namespace cinput

#endif // CINPUT_HPP
