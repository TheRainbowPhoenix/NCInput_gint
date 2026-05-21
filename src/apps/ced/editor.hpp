#pragma once
#include <string>
#include <vector>
#include <memory>
#include "cinput.hpp"
#include "tokenizer.hpp"
#include "LazyFile.hpp"

namespace ced {

struct WrappedLineInfo {
    int visual_lines;
    int cursor_row;
};

class Editor {
public:
    Editor();

    void draw();
    void update(const std::vector<key_event_t>& events, bool& running);

    void insert_char(const std::string& s);
    void delete_char();
    void new_line();

    std::string do_menu();
    void load_file(const std::string& filename);
    void save_file(const std::string& target_name = "");

    void clamp_cursor();
    void scroll_to_cursor();

    // Public member access to mimic Python structure
    std::vector<std::string> lines;
    std::string filename;
    std::string current_theme_name;
    bool word_wrap;
    int cy, cx, vy;
    std::unique_ptr<cinput::Keyboard> keyboard;
    std::string msg;
    int msg_timer;

private:
    WrappedLineInfo get_wrapped_line_info(const std::string& line);
    std::pair<int, int> get_cursor_rect(const std::string& line, int cx);
    int get_cx_from_px(const std::string& line, int target_x, int target_visual_row);

    void update_colors();
    void switch_theme();

    void draw_header();
    void draw_text_content(int view_h);
    void draw_indentation_guides(const std::string& line, int x, int y);
    void draw_icon_menu(int x, int y, color_t col);
    void draw_icon_kbd(int x, int y, color_t col);

    bool touch_latched;
};

} // namespace ced
