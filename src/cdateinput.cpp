#include "cdateinput.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <map>
#include <string>

namespace cdateinput {

std::vector<std::string> DAY_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
std::vector<std::string> DOW_SINGLE = {"S", "M", "T", "W", "T", "F", "S"};
std::vector<std::string> MONTH_NAMES = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
std::vector<std::string> MONTH_NAMES_LONG = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

static std::map<int, int> DIGIT_KEYS = {
    {KEY_0, 0}, {KEY_1, 1}, {KEY_2, 2}, {KEY_3, 3}, {KEY_4, 4},
    {KEY_5, 5}, {KEY_6, 6}, {KEY_7, 7}, {KEY_8, 8}, {KEY_9, 9}
};

bool is_leap(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int days_in_month(int year, int month) {
    if (month == 2) {
        return is_leap(year) ? 29 : 28;
    }
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return days[month - 1];
}

int day_of_week(int year, int month, int day) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year -= 1;
    return (year + year/4 - year/100 + year/400 + t[month-1] + day) % 7;
}

void add_days(int& y, int& m, int& d, int delta) {
    while (delta > 0) {
        int left = days_in_month(y, m) - d;
        if (delta <= left) {
            d += delta;
            delta = 0;
        } else {
            delta -= (left + 1);
            d = 1;
            m += 1;
            if (m > 12) { m = 1; y += 1; }
        }
    }
    while (delta < 0) {
        if (d > -delta) {
            d += delta;
            delta = 0;
        } else {
            delta += d;
            m -= 1;
            if (m < 1) { m = 12; y -= 1; }
            d = days_in_month(y, m);
        }
    }
}

DatePicker::DatePicker(const std::string& prompt, int default_y, int default_m, int default_d,
                       const std::string& theme, Date* min_date, Date* max_date)
    : m_prompt(prompt), m_min_date(min_date), m_max_date(max_date), m_theme_name(theme), m_theme(cinput::get_theme(theme))
{
    m_current = clamp_date(default_y, default_m, default_d);
    btn_ok_pressed = btn_cn_pressed = left_arrow_pressed = right_arrow_pressed = false;
    header_h = 90;
    footer_h = 45;
    btn_w = SCREEN_W / 2;
}

Date DatePicker::clamp_date(int y, int m, int d) {
    Date current = {y, m, d};
    if (m_min_date && current < *m_min_date) return *m_min_date;
    if (m_max_date && current > *m_max_date) return *m_max_date;
    return current;
}

void DatePicker::draw_bold_text(int x, int y, color_t fg, const std::string& text) {
    dtext(x, y, fg, text.c_str());
    dtext(x + 1, y, fg, text.c_str());
}

void DatePicker::draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed) {
    color_t bg = pressed ? m_theme.hl : m_theme.key_spec;
    drect(x, y, x + w, y + h, bg);
    ::drect_border(x, y, x + w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);
    dtext_opt(x + w/2, y + h/2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, text.c_str(), -1);
}

void DatePicker::draw() {
    dclear(m_theme.modal_bg);
    drect(0, 0, SCREEN_W, header_h, m_theme.accent);
    dtext(20, 15, m_theme.txt_acc, m_prompt.c_str());

    int dow = day_of_week(m_current.year, m_current.month, m_current.day);
    char buf[64];
    sprintf(buf, "%s, %s %d", DAY_NAMES[dow].c_str(), MONTH_NAMES[m_current.month].c_str(), m_current.day);
    draw_bold_text(20, 45, m_theme.txt_acc, buf);
    sprintf(buf, "%d", m_current.year);
    draw_bold_text(20, 65, m_theme.txt_acc, buf);

    int my_y = header_h + 15;
    if (left_arrow_pressed) drect(240, my_y - 5, 275, my_y + 25, m_theme.hl);
    if (right_arrow_pressed) drect(275, my_y - 5, 315, my_y + 25, m_theme.hl);

    sprintf(buf, "%s %d", MONTH_NAMES_LONG[m_current.month].c_str(), m_current.year);
    dtext_opt(20, my_y + 10, m_theme.txt, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, buf, -1);

    int lx[] = {260, 265, 265}; int ly[] = {my_y+10, my_y+4, my_y+16};
    dpoly(lx, ly, 3, m_theme.txt, (int)C_NONE);
    int rx[] = {295, 290, 290}; int ry[] = {my_y+10, my_y+4, my_y+16};
    dpoly(rx, ry, 3, m_theme.txt, (int)C_NONE);

    int dow_y = my_y + 40;
    int spacing = 42;
    int offset_x = (SCREEN_W - 7 * spacing) / 2;
    for (int i = 0; i < 7; ++i) {
        int cx = offset_x + i * spacing + 21;
        dtext_opt(cx, dow_y, m_theme.txt_dim, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, DOW_SINGLE[i].c_str(), -1);
    }

    int grid_y = dow_y + 35;
    int start_dow = day_of_week(m_current.year, m_current.month, 1);
    int num_days = days_in_month(m_current.year, m_current.month);
    for (int d = 1; d <= num_days; ++d) {
        int pos = start_dow + d - 1;
        int row = pos / 7;
        int col = pos % 7;
        int cx = offset_x + col * spacing + 21;
        int cy = grid_y + row * 40;

        bool is_valid = true;
        Date d_obj = {m_current.year, m_current.month, d};
        if (m_min_date && d_obj < *m_min_date) is_valid = false;
        if (m_max_date && d_obj > *m_max_date) is_valid = false;

        if (!is_valid) {
            sprintf(buf, "%d", d);
            dtext_opt(cx, cy, m_theme.txt_dim, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
            continue;
        }

        sprintf(buf, "%d", d);
        if (d == m_current.day) {
            drect(cx - 16, cy - 16, cx + 16, cy + 16, m_theme.accent);
            dtext_opt(cx, cy, m_theme.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
        } else {
            dtext_opt(cx, cy, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
        }
    }

    int fy = SCREEN_H - footer_h;
    draw_btn(0, fy, btn_w, footer_h, "CANCEL", btn_cn_pressed);
    draw_btn(btn_w, fy, btn_w, footer_h, "OK", btn_ok_pressed);
}

Date* DatePicker::run() {
    clearevents();
    cleareventflips();
    while (true) {
        draw(); dupdate(); cleareventflips();
        key_event_t ev = pollevent();
        std::vector<key_event_t> events;
        while (ev.type != KEYEV_NONE) { events.push_back(ev); ev = pollevent(); }

        for (const auto& e : events) {
            if (e.type == KEYEV_DOWN && DIGIT_KEYS.count(e.key)) {
                int digit = DIGIT_KEYS[e.key];
                std::string new_str = num_buffer + std::to_string(digit);
                if (std::stoi(new_str) > days_in_month(m_current.year, m_current.month)) new_str = std::to_string(digit);
                if (new_str != "0") {
                    m_current = clamp_date(m_current.year, m_current.month, std::stoi(new_str));
                    num_buffer = new_str;
                    if (num_buffer.length() >= 2) num_buffer = "";
                } else num_buffer = "0";
            } else if (e.type == KEYEV_DOWN || e.type == KEYEV_TOUCH_DOWN) num_buffer = "";
        }

        if (keypressed(KEY_EXIT) || keypressed(KEY_DEL)) return nullptr;
        if (keypressed(KEY_EXE)) return new Date(m_current);
        if (keypressed(KEY_LEFT)) { add_days(m_current.year, m_current.month, m_current.day, -1); m_current = clamp_date(m_current.year, m_current.month, m_current.day); }
        else if (keypressed(KEY_RIGHT)) { add_days(m_current.year, m_current.month, m_current.day, 1); m_current = clamp_date(m_current.year, m_current.month, m_current.day); }
        else if (keypressed(KEY_UP)) { add_days(m_current.year, m_current.month, m_current.day, -7); m_current = clamp_date(m_current.year, m_current.month, m_current.day); }
        else if (keypressed(KEY_DOWN)) { add_days(m_current.year, m_current.month, m_current.day, 7); m_current = clamp_date(m_current.year, m_current.month, m_current.day); }

        for (const auto& e : events) {
            int fy = SCREEN_H - footer_h;
            if (e.type == KEYEV_TOUCH_DOWN) {
                if (e.y >= fy) { if (e.x < btn_w) btn_cn_pressed = true; else btn_ok_pressed = true; }
                else if (header_h < e.y && e.y < header_h + 50) { if (240 <= e.x && e.x < 275) left_arrow_pressed = true; else if (e.x >= 275) right_arrow_pressed = true; }
            } else if (e.type == KEYEV_TOUCH_UP) {
                if (e.y >= fy) {
                    if (btn_ok_pressed && e.x >= btn_w) return new Date(m_current);
                    if (btn_cn_pressed && e.x < btn_w) return nullptr;
                } else if (header_h < e.y && e.y < header_h + 50) {
                    if (left_arrow_pressed && 240 <= e.x && e.x < 275) {
                        m_current.month--; if (m_current.month < 1) { m_current.month = 12; m_current.year--; }
                        m_current.day = std::min(m_current.day, days_in_month(m_current.year, m_current.month));
                        m_current = clamp_date(m_current.year, m_current.month, m_current.day);
                    } else if (right_arrow_pressed && e.x >= 275) {
                        m_current.month++; if (m_current.month > 12) { m_current.month = 1; m_current.year++; }
                        m_current.day = std::min(m_current.day, days_in_month(m_current.year, m_current.month));
                        m_current = clamp_date(m_current.year, m_current.month, m_current.day);
                    } else if (e.x < 200) {
                        cinput::PickResult target = cinput::pick({"Month", "Year"}, "Edit", m_theme_name);
                        if (target.success && !target.values.empty()) {
                            if (target.values[0] == "Month") {
                                std::vector<std::string> months(MONTH_NAMES_LONG.begin() + 1, MONTH_NAMES_LONG.end());
                                cinput::PickResult res = cinput::pick(months, "Month", m_theme_name);
                                if (res.success && !res.values.empty()) {
                                    for (int i = 1; i <= 12; ++i) if (MONTH_NAMES_LONG[i] == res.values[0]) { m_current.month = i; break; }
                                }
                            } else {
                                int ymin = m_min_date ? m_min_date->year : 2011, ymax = m_max_date ? m_max_date->year : 2048;
                                std::vector<std::string> years; for (int y = ymin; y <= ymax; ++y) years.push_back(std::to_string(y));
                                cinput::PickResult res = cinput::pick(years, "Year", m_theme_name);
                                if (res.success && !res.values.empty()) m_current.year = std::stoi(res.values[0]);
                            }
                        }
                        m_current.day = std::min(m_current.day, days_in_month(m_current.year, m_current.month));
                        m_current = clamp_date(m_current.year, m_current.month, m_current.day);
                        clearevents();
                    }
                } else if (e.y > header_h + 80 && e.y < fy) {
                    int spacing = 42, offset_x = (SCREEN_W - 7 * spacing) / 2, grid_y_top = header_h + 55 + 35;
                    int col = (e.x - offset_x) / spacing, row = (e.y - grid_y_top + 20) / 40;
                    if (0 <= col && col < 7 && 0 <= row && row < 6) {
                        int start_dow_click = day_of_week(m_current.year, m_current.month, 1);
                        int clicked_day = row * 7 + col - start_dow_click + 1;
                        if (1 <= clicked_day && clicked_day <= days_in_month(m_current.year, m_current.month))
                            m_current = clamp_date(m_current.year, m_current.month, clicked_day);
                    }
                }
                btn_ok_pressed = btn_cn_pressed = left_arrow_pressed = right_arrow_pressed = false;
            }
        }
        sleep_ms(10);
    }
}

TimePicker::TimePicker(const std::string& prompt, int default_h, int default_m, const std::string& theme, Time* min_time, Time* max_time)
    : m_prompt(prompt), m_min_time(min_time), m_max_time(max_time), m_theme(cinput::get_theme(theme))
{
    offset_h = (float)default_h; offset_m = (float)default_m; enforce_boundaries();
    btn_ok_pressed = btn_cn_pressed = false;
    header_h = 90; footer_h = 45; item_h = 45; btn_w = SCREEN_W / 2;
    center_y = (SCREEN_H - header_h - footer_h) / 2 + header_h;
    focus_col = 0;
}

void TimePicker::enforce_boundaries() {
    if (m_min_time || m_max_time) {
        if (m_min_time) {
            if (offset_h < m_min_time->hour) offset_h = (float)m_min_time->hour;
            if (offset_h <= m_min_time->hour && offset_m < m_min_time->minute) offset_m = (float)m_min_time->minute;
        }
        if (m_max_time) {
            if (offset_h > m_max_time->hour) offset_h = (float)m_max_time->hour;
            if (offset_h >= m_max_time->hour && offset_m > m_max_time->minute) offset_m = (float)m_max_time->minute;
        }
    } else {
        while (offset_h < 0) offset_h += 24; while (offset_h >= 24) offset_h -= 24;
        while (offset_m < 0) offset_m += 60; while (offset_m >= 60) offset_m -= 60;
    }
}

void TimePicker::draw_bold_text(int x, int y, color_t fg, const std::string& text) {
    dtext(x, y, fg, text.c_str()); dtext(x + 1, y, fg, text.c_str());
}

void TimePicker::draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed) {
    color_t bg = pressed ? m_theme.hl : m_theme.key_spec;
    drect(x, y, x + w, y + h, bg);
    ::drect_border(x, y, x + w, y + h, (int)C_NONE, 1, (int)m_theme.key_spec);
    dtext_opt(x + w/2, y + h/2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, text.c_str(), -1);
}

void TimePicker::draw() {
    dclear(m_theme.modal_bg);
    drect(0, 0, SCREEN_W, header_h, m_theme.accent);
    dtext(20, 15, m_theme.txt_acc, m_prompt.c_str());
    char buf[32]; sprintf(buf, "%02d:%02d", (int)round(offset_h) % 24, (int)round(offset_m) % 60);
    draw_bold_text(20, 50, m_theme.txt_acc, buf);

    int band_y1 = center_y - item_h / 2, band_y2 = center_y + item_h / 2;
    drect(0, band_y1, SCREEN_W, band_y2, m_theme.hl);
    dhline(band_y1, m_theme.key_spec); dhline(band_y2, m_theme.key_spec);
    if (focus_col == 0) ::drect_border(30, band_y1, 130, band_y2, (int)C_NONE, 1, (int)m_theme.txt_dim);
    else ::drect_border(190, band_y1, 290, band_y2, (int)C_NONE, 1, (int)m_theme.txt_dim);

    struct Col { float off; int mod; int cx; };
    Col cols[] = {{offset_h, 24, 80}, {offset_m, 60, 240}};
    for (int ci = 0; ci < 2; ++ci) {
        int base = (int)floor(cols[ci].off); float rem = cols[ci].off - base;
        for (int i = -3; i <= 3; ++i) {
            int val = base + i;
            if (m_min_time || m_max_time) {
                if (ci == 0 && (val < 0 || val > 23)) continue;
                if (ci == 1 && (val < 0 || val > 59)) continue;
                if (ci == 0) {
                    if (m_min_time && val < m_min_time->hour) continue;
                    if (m_max_time && val > m_max_time->hour) continue;
                } else {
                    int ch = (int)round(offset_h);
                    if (m_min_time && ch == m_min_time->hour && val < m_min_time->minute) continue;
                    if (m_max_time && ch == m_max_time->hour && val > m_max_time->minute) continue;
                }
            } else { val = (val % cols[ci].mod + cols[ci].mod) % cols[ci].mod; }
            int cy = center_y + (int)((i - rem) * item_h);
            color_t fg = (abs(i - rem) < 0.5) ? m_theme.txt : m_theme.txt_dim;
            if (header_h < cy && cy < SCREEN_H - footer_h) {
                sprintf(buf, "%02d", val);
                dtext_opt(cols[ci].cx, cy, fg, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
            }
        }
    }
    dtext_opt(SCREEN_W/2, center_y - 2, m_theme.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, ":", -1);
    int fy = SCREEN_H - footer_h;
    draw_btn(0, fy, btn_w, footer_h, "CANCEL", btn_cn_pressed);
    draw_btn(btn_w, fy, btn_w, footer_h, "OK", btn_ok_pressed);
}

Time* TimePicker::run() {
    clearevents(); cleareventflips();
    bool is_dragging = false; int last_y = 0;
    while (true) {
        draw(); dupdate(); cleareventflips();
        key_event_t ev = pollevent(); std::vector<key_event_t> events;
        while (ev.type != KEYEV_NONE) { events.push_back(ev); ev = pollevent(); }
        for (const auto& e : events) {
            if (e.type == KEYEV_DOWN && DIGIT_KEYS.count(e.key)) {
                int digit = DIGIT_KEYS[e.key], limit = (focus_col == 0 ? 23 : 59);
                std::string ns = num_buffer + std::to_string(digit);
                if (std::stoi(ns) > limit) ns = std::to_string(digit);
                if (focus_col == 0) offset_h = (float)std::stoi(ns); else offset_m = (float)std::stoi(ns);
                enforce_boundaries(); num_buffer = ns; if (num_buffer.length() >= 2) num_buffer = "";
            } else if (e.type == KEYEV_DOWN || e.type == KEYEV_TOUCH_DOWN) num_buffer = "";
        }
        if (keypressed(KEY_EXIT) || keypressed(KEY_DEL)) return nullptr;
        if (keypressed(KEY_EXE)) return new Time{(int)round(offset_h) % 24, (int)round(offset_m) % 60};
        if (keypressed(KEY_LEFT)) focus_col = 0; else if (keypressed(KEY_RIGHT)) focus_col = 1;
        else if (keypressed(KEY_UP)) { if (focus_col == 0) offset_h -= 1; else offset_m -= 1; enforce_boundaries(); }
        else if (keypressed(KEY_DOWN)) { if (focus_col == 0) offset_h += 1; else offset_m += 1; enforce_boundaries(); }
        for (const auto& e : events) {
            int fy = SCREEN_H - footer_h;
            if (e.type == KEYEV_TOUCH_DOWN) {
                if (e.y >= fy) { if (e.x < btn_w) btn_cn_pressed = true; else btn_ok_pressed = true; }
                else if (header_h < e.y && e.y < fy) { is_dragging = true; last_y = e.y; focus_col = (e.x < SCREEN_W / 2 ? 0 : 1); }
            } else if (e.type == KEYEV_TOUCH_DRAG && is_dragging) {
                float dy = (float)(e.y - last_y);
                if (focus_col == 0) offset_h -= dy / item_h; else offset_m -= dy / item_h;
                enforce_boundaries(); last_y = e.y;
            } else if (e.type == KEYEV_TOUCH_UP) {
                if (is_dragging) { offset_h = round(offset_h); offset_m = round(offset_m); enforce_boundaries(); is_dragging = false; }
                else if (e.y >= fy) {
                    if (btn_ok_pressed && e.x >= btn_w) return new Time{(int)round(offset_h) % 24, (int)round(offset_m) % 60};
                    if (btn_cn_pressed && e.x < btn_w) return nullptr;
                }
                btn_ok_pressed = btn_cn_pressed = false;
            }
        }
        sleep_ms(10);
    }
}

Date* datepick(const std::string& prompt, int default_y, int default_m, int default_d, const std::string& theme, Date* min_date, Date* max_date) {
    DatePicker picker(prompt, default_y, default_m, default_d, theme, min_date, max_date); return picker.run();
}

Time* timepick(const std::string& prompt, int default_h, int default_m, const std::string& theme, Time* min_time, Time* max_time) {
    TimePicker picker(prompt, default_h, default_m, theme, min_time, max_time); return picker.run();
}

} // namespace cdateinput
