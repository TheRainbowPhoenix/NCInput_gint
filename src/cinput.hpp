#ifndef CINPUT_HPP
#define CINPUT_HPP

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include "cinput_utils.hpp"

namespace cinput {

// =============================================================================
// CONSTANTS & CONFIG
// =============================================================================

const int SCREEN_W = 320;
const int SCREEN_H = 528;

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

const Theme& get_theme(const String& name);

// =============================================================================
// REUSABLE LIST VIEW
// =============================================================================

struct ListItem {
    String text;
    String type;
    int height;
    bool checked;
    bool arrow;
    int idx;

    // Internal layout state
    int _h;
    int _y;

    ListItem() : type("item"), height(0), checked(false), arrow(false), idx(-1), _h(0), _y(0) {}
};

class ListView {
public:
    struct Rect { int x, y, w, h; };
    struct Action {
        String type; // "click", "long"
        int index;
        ListItem item;
        Action() : index(-1) {}
    };

    ListView(Rect rect, const Vector<ListItem>& items, int row_h = 40, const String& theme = "light", int headers_h = -1);

    void recalc_layout();
    void select_next(int start_idx, int step);
    void ensure_visible();
    void clamp_scroll();

    bool update(const Vector<key_event_t>& events, Action& out_action);

    int get_index_at(int y);
    void draw_item(int x, int y, const ListItem& item, bool is_selected);
    void draw();
    void draw_check(int x, int y, const Theme& t);

    Rect m_rect;
    Vector<ListItem> m_items;
    int m_base_row_h;
    int m_headers_h;
    const Theme& m_theme;

    int m_total_h;
    int m_selected_index;
    int m_scroll_y;
    int m_max_scroll;

    // Touch State
    bool m_is_dragging;
    int m_touch_start_y;
    int m_touch_start_idx;
    uint32_t m_touch_start_ticks;
    int m_touch_initial_item_idx;
    bool m_long_press_triggered;

    // Configuration
    int m_drag_threshold;
    uint32_t m_long_press_delay_ticks;
};

// =============================================================================
// KEYBOARD WIDGET
// =============================================================================

struct NumpadOpts {
    bool is_float;
    bool is_neg;
    NumpadOpts() : is_float(true), is_neg(true) {}
    NumpadOpts(bool f, bool n) : is_float(f), is_neg(n) {}
};

struct KeyRect {
    int x, y, w, h;
    String label;
    String val;
    bool is_spec;
    bool is_acc;

    KeyRect() : x(0), y(0), w(0), h(0), is_spec(false), is_acc(false) {}
    KeyRect(int _x, int _y, int _w, int _h, String _l, String _v, bool _s, bool _a)
        : x(_x), y(_y), w(_w), h(_h), label(_l), val(_v), is_spec(_s), is_acc(_a) {}
};

class Keyboard {
public:
    Keyboard(int default_tab = 0, bool enable_tabs = true, NumpadOpts numpad_opts = NumpadOpts(), const String& theme = "light", const String& layout = "qwerty");

    void draw_key(int x, int y, int w, int h, const String& label, bool is_special = false, bool is_pressed = false, bool is_accent = false);
    void draw_tabs();
    void draw_grid();
    String update_grid(int x, int y, int type);

    Vector<KeyRect> get_math_rects();
    Vector<KeyRect> get_numpad_rects();
    void draw_keys_from_rects(const Vector<KeyRect>& rects);
    String update_keys_from_rects(const Vector<KeyRect>& rects, int x, int y, int type);

    void draw();
    String update(const key_event_t& ev);

    int m_y;
    bool m_visible;
    int m_current_tab;
    bool m_enable_tabs;
    bool m_shift;
    String m_tabs[3];
    String m_last_key;
    NumpadOpts m_numpad_opts;
    const Theme& m_theme;
    Vector<Vector<String>> m_layout_alpha;
};

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

struct InputResult {
    bool success;
    String value;
};

struct PickResult {
    bool success;
    Vector<String> values;
};

InputResult input(const String& prompt = "Input:", const String& type = "alpha_numeric", const String& theme = "light", const String& layout = "qwerty", int touch_mode = KEYEV_TOUCH_DOWN);
PickResult pick(const Vector<String>& options, const String& prompt = "Select:", const String& theme = "light", bool multi = false, int touch_mode = KEYEV_TOUCH_DOWN);

// =============================================================================
// LIST PICKER WIDGET
// =============================================================================

class ListPicker {
public:
    ListPicker(const Vector<String>& options, const String& prompt = "Select:", const String& theme = "light", bool multi = false, int touch_mode = KEYEV_TOUCH_DOWN);
    ~ListPicker();

    void draw_nav_btn(int x, int w, int h, const String& type, bool is_pressed);
    void draw_close_icon(int x, int y, int sz, color_t col);
    void draw();
    PickResult run();

    Vector<String> m_options;
    String m_prompt;
    const Theme& m_theme;
    String m_theme_name;
    bool m_multi;
    int m_touch_mode;
    Vector<int> m_selected_indices; // for multi
    int m_single_selection;

    int m_header_h;
    int m_footer_h;
    int m_view_h;
    int m_btn_w;
    String m_last_action;

    UniquePtr<ListView> m_list_view;
};

// =============================================================================
// CONFIRMATION DIALOG
// =============================================================================

class ConfirmationDialog {
public:
    ConfirmationDialog(const String& title, const String& body, const String& ok_text = "OK", const String& cancel_text = "Cancel", const String& theme = "light", int touch_mode = KEYEV_TOUCH_DOWN);

    void draw_btn(int x, int y, int w, int h, const String& text, bool pressed);
    void draw(bool btn_ok_pressed, bool btn_cn_pressed);
    bool run();

    String m_title;
    String m_body;
    String m_ok_text;
    String m_cancel_text;
    const Theme& m_theme;
    int m_touch_mode;
    int m_header_h;
    int m_footer_h;
    int m_btn_w;
};

bool ask(const String& title, const String& body, const String& ok_text = "OK", const String& cancel_text = "Cancel", const String& theme = "light", int touch_mode = KEYEV_TOUCH_DOWN);

} // namespace cinput

#endif // CINPUT_HPP
