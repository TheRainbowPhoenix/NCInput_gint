#include "editor.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace ced {

const int HEADER_H = 40;
const int TEXT_LINE_H = 20;
const int TEXT_MARGIN_X = 5;
const int TEXT_Y_OFFSET = 4;

Editor::Editor() {
    lines = {""};
    filename = "untitled.py";
    current_theme_name = "light";
    word_wrap = false;
    cy = 0; cx = 0; vy = 0;
    keyboard = std::make_unique<cinput::Keyboard>(current_theme_name, "qwerty");
    keyboard->visible = false;
    msg = "Welcome to CED";
    msg_timer = 100;
    touch_latched = false;
}

void Editor::clamp_cursor() {
    if (cy < 0) cy = 0;
    if (cy >= (int)lines.size()) cy = lines.size() - 1;

    int line_len = lines[cy].length();
    if (cx < 0) cx = 0;
    if (cx > line_len) cx = line_len;
}

WrappedLineInfo Editor::get_wrapped_line_info(const std::string& line) {
    if (!word_wrap) return {1, 0};

    Tokenizer tk(line, current_theme_name);
    std::string text; color_t color;
    int cur_x = TEXT_MARGIN_X;
    int visual_lines = 1;
    int cursor_row = -1;
    int char_count = 0;

    if (cx == 0) cursor_row = 0;

    while (tk.next(text, color)) {
        int t_w, t_h;
        dsize(text.c_str(), NULL, &t_w, &t_h);

        int token_len = text.length();

        // If the token itself is too wide, we need to wrap char by char
        if (cur_x + t_w > SCREEN_W - 5) {
            for(int i = 0; i < token_len; ++i) {
                int cw, ch;
                char c_str[2] = {text[i], 0};
                dsize(c_str, NULL, &cw, &ch);

                if (cur_x + cw > SCREEN_W - 5) {
                    visual_lines++;
                    cur_x = TEXT_MARGIN_X;
                }

                char_count++;
                if (cursor_row == -1 && cx == char_count) {
                    cursor_row = visual_lines - 1;
                }
                cur_x += cw;
            }
        } else {
            if (cursor_row == -1) {
                if (cx > char_count && cx <= char_count + token_len) {
                    // Estimate cursor_row more accurately if token wraps
                    // But here the whole token fits in cur_x + t_w
                    cursor_row = visual_lines - 1;
                }
            }
            cur_x += t_w;
            char_count += token_len;
        }
    }

    if (cursor_row == -1 && cx == char_count) cursor_row = visual_lines - 1;

    return {visual_lines, cursor_row};
}

void Editor::scroll_to_cursor() {
    int kb_h = keyboard->visible ? 260 : 0;
    int view_h = SCREEN_H - HEADER_H - kb_h;

    if (cy < vy) {
        vy = cy;
        return;
    }

    if (cy > vy + 100) vy = cy - 5;

    while (true) {
        int current_visual_h = 0;
        bool cursor_is_visible = false;

        for (int i = vy; i < (int)lines.size(); ++i) {
            int line_h;
            if (!word_wrap) {
                line_h = TEXT_LINE_H;
                if (i == cy && current_visual_h + line_h <= view_h) cursor_is_visible = true;
            } else {
                auto info = get_wrapped_line_info(lines[i]);
                line_h = info.visual_lines * TEXT_LINE_H;
                if (i == cy) {
                    int cursor_bottom = current_visual_h + (info.cursor_row + 1) * TEXT_LINE_H;
                    if (cursor_bottom <= view_h) cursor_is_visible = true;
                }
            }
            current_visual_h += line_h;
            if (i >= cy || (current_visual_h > view_h && i < cy)) break;
        }

        if (cursor_is_visible) break;
        else {
            vy++;
            if (vy > cy) { vy = cy; break; }
        }
    }
}

std::string Editor::do_menu() {
    std::string wrap_status = word_wrap ? "On" : "Off";
    std::vector<std::string> opts = {
        "New", "Save", "Save As...", "Open...",
        "Word Wrap: " + wrap_status,
        "Theme: " + current_theme_name,
        "Quit"
    };
    cinput::PickResult res = cinput::pick(opts, "Menu", current_theme_name);
    if (!res.success || res.values.empty()) return "";

    std::string choice = res.values[0];
    if (choice == "New") {
        lines = {""}; filename = "untitled.py"; cy = 0; cx = 0;
    } else if (choice == "Save") {
        save_file();
    } else if (choice == "Save As...") {
        cinput::InputResult ir = cinput::input("Save as:", "text", current_theme_name);
        if (ir.success) save_file(ir.value);
    } else if (choice == "Open...") {
        cinput::InputResult ir = cinput::input("File to open:", "text", current_theme_name);
        if (ir.success) load_file(ir.value);
    } else if (choice.find("Word Wrap") != std::string::npos) {
        word_wrap = !word_wrap;
    } else if (choice.find("Theme") != std::string::npos) {
        switch_theme();
    } else if (choice == "Quit") {
        return "QUIT";
    }
    return "";
}

void Editor::switch_theme() {
    std::vector<std::string> themes = {"light", "dark", "grey"};
    auto it = std::find(themes.begin(), themes.end(), current_theme_name);
    int idx = (it == themes.end()) ? 0 : std::distance(themes.begin(), it);
    current_theme_name = themes[(idx + 1) % themes.size()];
    bool vis = keyboard->visible;
    keyboard = std::make_unique<cinput::Keyboard>(current_theme_name, "qwerty");
    keyboard->visible = vis;
}

void Editor::load_file(const std::string& fname) {
    LazyFile lf(fname, "r");
    auto f = lf.acquire();
    if (!f) {
        msg = "Error loading " + fname;
        msg_timer = 60;
        return;
    }

    lines.clear();
    char buf[1024];
    std::string current_line;
    while (std::fgets(buf, sizeof(buf), f.get())) {
        for (int i = 0; buf[i]; ++i) {
            if (buf[i] == '\n') {
                lines.push_back(current_line);
                current_line.clear();
            } else if (buf[i] != '\r') {
                current_line.push_back(buf[i]);
            }
        }
    }
    lines.push_back(current_line);
    if (lines.empty()) lines = {""};
    filename = fname;
    msg = "Loaded " + fname;
    msg_timer = 60;
    cy = 0; cx = 0; vy = 0;
}

void Editor::save_file(const std::string& target_name) {
    std::string target = target_name.empty() ? filename : target_name;
    if (target == "untitled.py") {
        cinput::InputResult ir = cinput::input("Save as:", "text", current_theme_name);
        if (!ir.success) return;
        target = ir.value;
    }

    LazyFile lf(target, "w");
    auto f = lf.acquire();
    if (!f) {
        msg = "Error saving file";
        msg_timer = 60;
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        std::fputs(lines[i].c_str(), f.get());
        if (i < lines.size() - 1) std::fputc('\n', f.get());
    }
    filename = target;
    msg = "Saved " + target;
    msg_timer = 60;
}

void Editor::insert_char(const std::string& s) {
    clamp_cursor();
    lines[cy].insert(cx, s);
    cx += s.length();
    clamp_cursor();
}

void Editor::delete_char() {
    clamp_cursor();
    if (cx > 0) {
        lines[cy].erase(cx - 1, 1);
        cx--;
    } else if (cy > 0) {
        std::string curr = lines[cy];
        lines.erase(lines.begin() + cy);
        cy--;
        cx = lines[cy].length();
        lines[cy] += curr;
    }
    clamp_cursor();
}

void Editor::new_line() {
    clamp_cursor();
    std::string rem = lines[cy].substr(cx);
    lines[cy] = lines[cy].substr(0, cx);
    cy++;
    lines.insert(lines.begin() + cy, rem);
    cx = 0;
    clamp_cursor();
}

std::pair<int, int> Editor::get_cursor_rect(const std::string& line, int cx) {
    Tokenizer tk(line, current_theme_name);
    std::string text; color_t color;
    int cur_x = TEXT_MARGIN_X;
    int cur_y = 0;
    int char_count = 0;

    if (cx == 0) return {cur_x, cur_y};

    while (tk.next(text, color)) {
        int t_w, t_h;
        dsize(text.c_str(), NULL, &t_w, &t_h);
        int token_len = text.length();

        if (word_wrap && cur_x + t_w > SCREEN_W - 5) {
             for(int i = 0; i < token_len; ++i) {
                int cw, ch;
                char c_str[2] = {text[i], 0};
                dsize(c_str, NULL, &cw, &ch);
                if (cur_x + cw > SCREEN_W - 5) {
                    cur_y += TEXT_LINE_H;
                    cur_x = TEXT_MARGIN_X;
                }
                char_count++;
                if (cx == char_count) return {cur_x + cw, cur_y};
                cur_x += cw;
            }
        } else {
            if (cx >= char_count && cx <= char_count + token_len) {
                int local_off = cx - char_count;
                int sub_w, sub_h;
                dsize(text.substr(0, local_off).c_str(), NULL, &sub_w, &sub_h);
                return {cur_x + sub_w, cur_y};
            }
            cur_x += t_w;
            char_count += token_len;
        }
    }
    return {cur_x, cur_y};
}

int Editor::get_cx_from_px(const std::string& line, int target_x, int target_visual_row) {
    Tokenizer tk(line, current_theme_name);
    std::string text; color_t color;
    int cur_x = TEXT_MARGIN_X;
    int cur_vis_row = 0;
    int char_count = 0;

    while (tk.next(text, color)) {
        int t_w, t_h;
        dsize(text.c_str(), NULL, &t_w, &t_h);

        if (word_wrap && cur_x + t_w > SCREEN_W - 5) {
            if (cur_vis_row == target_visual_row) return char_count;
            cur_vis_row++;
            cur_x = TEXT_MARGIN_X;
        }

        if (cur_vis_row == target_visual_row) {
            if (target_x < cur_x + t_w) {
                int rel_x = std::max(0, target_x - cur_x);
                int fit_w;
                const char* next_p = drsize(text.c_str(), NULL, rel_x, &fit_w);
                int offset = (next_p) ? (int)(next_p - text.c_str()) : (int)text.length();

                if (offset < (int)text.length()) {
                    int cw, ch; dsize(text.substr(offset, 1).c_str(), NULL, &cw, &ch);
                    if (rel_x > fit_w + (cw / 2)) offset++;
                }
                return char_count + offset;
            }
        }
        cur_x += t_w;
        char_count += text.length();
    }
    return char_count;
}

void Editor::draw_icon_menu(int x, int y, color_t col) {
    for (int i = 0; i < 3; ++i) drect(x, y + 4 + i * 5, x + 18, y + 5 + i * 5, col);
}

void Editor::draw_icon_kbd(int x, int y, color_t col) {
    ::drect_border(x, y + 2, x + 22, y + 16, (int)C_NONE, 1, (int)col);
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 3; ++c) {
            int px = x + 3 + c * 6, py = y + 5 + r * 5;
            drect(px, py, px + 3, py + 2, col);
        }
    }
}

void Editor::draw_indentation_guides(const std::string& line, int x, int y) {
    int spaces = 0;
    for (char c : line) { if (c == ' ') spaces++; else break; }
    color_t guide_col = (current_theme_name == "light") ? 0xCE59 : 0x4208;
    int sw, sh; dsize(" ", NULL, &sw, &sh);
    int block_w = sw * 4;
    for (int t = 0; t < spaces / 4; ++t) {
        int gx = x + t * block_w;
        drect(gx, y + TEXT_LINE_H - 2, gx + block_w - 2, y + TEXT_LINE_H - 1, guide_col);
    }
}

void Editor::draw_text_content(int view_h) {
    dwindow_set({0, HEADER_H, SCREEN_W, SCREEN_H + HEADER_H}); // Fixed height issue
    int current_screen_y = HEADER_H + 6;
    int max_y = HEADER_H + view_h;
    color_t col_txt = cinput::get_theme(current_theme_name).txt;

    for (int i = vy; i < (int)lines.size(); ++i) {
        if (current_screen_y >= max_y) break;
        const std::string& line = lines[i];
        Tokenizer tk(line, current_theme_name);
        std::string text; color_t color;
        draw_indentation_guides(line, TEXT_MARGIN_X, current_screen_y);

        if (i == cy) {
            auto pos = get_cursor_rect(line, cx);
            int cursor_screen_y = current_screen_y + pos.second;
            if (cursor_screen_y < max_y && cursor_screen_y >= HEADER_H) {
                drect(pos.first, cursor_screen_y, pos.first + 2, cursor_screen_y + TEXT_LINE_H - 2, col_txt);
            }
        }

        int cur_x = TEXT_MARGIN_X;
        while (tk.next(text, color)) {
            int tw, th; dsize(text.c_str(), NULL, &tw, &th);
            int token_len = text.length();

            if (word_wrap && cur_x + tw > SCREEN_W - 5) {
                // Char by char drawing for wrapping tokens
                for(int j = 0; j < token_len; ++j) {
                    char c_str[2] = {text[j], 0};
                    int cw, ch; dsize(c_str, NULL, &cw, &ch);
                    if (cur_x + cw > SCREEN_W - 5) {
                        current_screen_y += TEXT_LINE_H; cur_x = TEXT_MARGIN_X;
                        if (current_screen_y >= max_y) goto next_line;
                    }
                    dtext(cur_x, current_screen_y + TEXT_Y_OFFSET, color, c_str);
                    cur_x += cw;
                }
            } else {
                if (word_wrap || cur_x <= SCREEN_W) {
                    dtext(cur_x, current_screen_y + TEXT_Y_OFFSET, color, text.c_str());
                }
                cur_x += tw;
            }
        }
        current_screen_y += TEXT_LINE_H;
        next_line:;
    }
    dwindow_set({0, 0, SCREEN_W, SCREEN_H});
}

void Editor::draw() {
    const auto& t = cinput::get_theme(current_theme_name);
    dclear(t.modal_bg);
    drect(0, 0, SCREEN_W, HEADER_H, t.accent);
    draw_icon_menu(10, 10, t.txt_acc);
    int kbd_x = SCREEN_W - 35;
    draw_icon_kbd(kbd_x, 10, t.txt_acc);
    if (keyboard->visible) drect(kbd_x, 22, kbd_x + 22, 23, t.txt_acc);
    dtext_opt(SCREEN_W / 2, HEADER_H / 2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, filename.c_str(), -1);

    int kb_h = keyboard->visible ? 260 : 0;
    int view_h = SCREEN_H - HEADER_H - kb_h;
    draw_text_content(view_h);

    if (msg_timer > 0) {
        msg_timer--;
        dtext(10, SCREEN_H - kb_h - 20, C_RED, msg.c_str());
    }
    keyboard->draw();
    dupdate();
}

void Editor::update(const std::vector<key_event_t>& events, bool& running) {
    for (const auto& e : events) {
        if (e.type == KEYEV_DOWN) {
            if (e.key == KEY_MENU || e.key == KEY_F1) {
                if (do_menu() == "QUIT") running = false;
                return;
            } else if (e.key == KEY_EXIT) {
                keyboard->visible = !keyboard->visible;
            } else if (e.key == KEY_UP) {
                cy--; clamp_cursor(); scroll_to_cursor();
            } else if (e.key == KEY_DOWN) {
                cy++; clamp_cursor(); scroll_to_cursor();
            } else if (e.key == KEY_LEFT) {
                cx--; clamp_cursor();
            } else if (e.key == KEY_RIGHT) {
                cx++; clamp_cursor();
            } else if (e.key == KEY_EXE) {
                new_line(); scroll_to_cursor();
            } else if (e.key == KEY_DEL) {
                delete_char(); scroll_to_cursor();
            }
        }

        if (e.type == KEYEV_TOUCH_UP) {
            touch_latched = false;
            keyboard->last_key = "";
        } else if (e.type == KEYEV_TOUCH_DOWN) {
            if (!touch_latched) {
                touch_latched = true;
                if (e.y < HEADER_H) {
                    if (e.x < 40) { if (do_menu() == "QUIT") running = false; }
                    else if (e.x > SCREEN_W - 40) keyboard->visible = !keyboard->visible;
                } else if (!keyboard->visible || e.y < keyboard->y) {
                    int rel_y = e.y - (HEADER_H + 6);
                    int current_visual_y = 0;
                    int found_logic_row = -1;
                    int found_visual_row_offset = 0;
                    for (int i = vy; i < (int)lines.size(); ++i) {
                        int line_h;
                        if (!word_wrap) line_h = TEXT_LINE_H;
                        else { auto info = get_wrapped_line_info(lines[i]); line_h = info.visual_lines * TEXT_LINE_H; }
                        if (rel_y >= current_visual_y && rel_y < current_visual_y + line_h) {
                            found_logic_row = i;
                            found_visual_row_offset = (rel_y - current_visual_y) / TEXT_LINE_H;
                            break;
                        }
                        current_visual_y += line_h;
                    }
                    if (found_logic_row != -1) {
                        cy = found_logic_row;
                        cx = get_cx_from_px(lines[found_logic_row], e.x, found_visual_row_offset);
                        clamp_cursor();
                    }
                } else if (keyboard->visible) {
                    std::string res = keyboard->update(e);
                    if (!res.empty()) {
                        if (res == "ENTER") new_line();
                        else if (res == "BACKSPACE") delete_char();
                        else if (res != "CAPS" && res.length() == 1) insert_char(res);
                        scroll_to_cursor();
                    }
                }
            }
        }
    }
}

} // namespace ced
