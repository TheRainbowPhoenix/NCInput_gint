#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include "cinput.hpp"

// --- Colors ---
#define CP_BG         C_RGB(3, 3, 4)
#define CP_TAB_BG     C_RGB(6, 6, 8)
#define CP_TAB_HL     C_RGB(31, 20, 0)
#define CP_LCD_ON     C_WHITE
#define CP_LCD_OFF    C_RGB(5, 5, 5)
#define CP_LCD_ACCENT C_RGB(10, 20, 31)
#define CP_BTN_GREEN  C_RGB(8, 16, 4)
#define CP_BTN_RED    C_RGB(20, 5, 5)
#define CP_BTN_GREY   C_RGB(8, 8, 9)

static const uint8_t SEGMENTS[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
static const char* KEYPAD[4][3] = { {"1","2","3"}, {"4","5","6"}, {"7","8","9"}, {"DEL","0","START"} };

// --- State ---
struct AppState {
    int tab = 0;
    int press_id = 0;
    uint32_t press_time = 0;
    bool sw_running = false;
    uint32_t sw_start = 0;
    uint32_t sw_acc = 0;
    int tm_mode = 0; // 0: input, 1: running, 2: paused, 3: alert
    uint32_t tm_end = 0;
    uint32_t tm_rem = 0;
    uint32_t tm_alert_start = 0;
    int laps_cnt = 0;
} state;

struct Lap { uint32_t diff; uint32_t total; };
static Lap laps[30];
static int tm_input[6] = {0};

// --- Helpers ---
uint32_t ticks_ms() { return (rtc_ticks() * 1000) / 128; }

void set_visual_press(int id, uint32_t now) {
    state.press_id = id;
    state.press_time = now;
}

bool check_visual_press(int id, uint32_t now) {
    return (state.press_id == id && (now - state.press_time) < 150);
}

void draw_seg(int x, int y, int w, int h, bool is_horiz, color_t c) {
    if (is_horiz) {
        int t2 = h / 2;
        int px[] = { x, x + t2, x + w - t2, x + w, x + w - t2, x + t2 };
        int py[] = { y + t2, y, y, y + t2, y + h, y + h };
        dpoly(px, py, 6, c, (int)C_NONE);
    } else {
        int t2 = w / 2;
        int px[] = { x + t2, x + w, x + w, x + t2, x, x };
        int py[] = { y, y + t2, y + h - t2, y + h, y + h - t2, y + t2 };
        dpoly(px, py, 6, c, (int)C_NONE);
    }
}

void draw_digit(int x, int y, int w, int h, int t, int val, color_t on_col, color_t off_col) {
    uint8_t segs = (val >= 0 && val <= 9) ? SEGMENTS[val] : 0;
    auto draw = [&](int bit, int sx, int sy, int sw, int sh, bool horiz) {
        color_t c = (segs & (1 << bit)) ? on_col : off_col;
        if (c != C_NONE) draw_seg(sx, sy, sw, sh, horiz, c);
    };
    draw(0, x + t / 2, y, w - t, t, true);
    draw(1, x + w - t, y + t / 2, t, h / 2 - t / 2, false);
    draw(2, x + w - t, y + h / 2, t, h / 2 - t / 2, false);
    draw(3, x + t / 2, y + h - t, w - t, t, true);
    draw(4, x, y + h / 2, t, h / 2 - t / 2, false);
    draw(5, x, y + t / 2, t, h / 2 - t / 2, false);
    draw(6, x + t / 2, y + h / 2 - t / 2, w - t, t, true);
}

void draw_colon(int x, int y, int w, int h, color_t c) {
    drect(x, y + h / 3 - w / 2, x + w, y + h / 3 + w / 2, c);
    drect(x, y + 2 * h / 3 - w / 2, x + w, y + 2 * h / 3 + w / 2, c);
}

void draw_btn(int x, int y, int w, int h, const char* text, color_t color, bool pressed) {
    color_t bg = pressed ? (color_t)C_WHITE : color;
    color_t tc = pressed ? (color_t)C_BLACK : (color_t)C_WHITE;
    drect(x, y, x + w, y + h, bg);
    ::drect_border(x, y, x + w, y + h, (int)C_NONE, 2, (int)C_RGB(10, 10, 10));
    dtext_opt(x + w / 2, y + h / 2, tc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, text, -1);
}

void draw_tabs(int active) {
    drect(0, 0, SCREEN_W, 50, CP_TAB_BG);
    color_t c0 = (active == 0) ? C_WHITE : C_RGB(15, 15, 15);
    color_t c1 = (active == 1) ? C_WHITE : C_RGB(15, 15, 15);
    dtext_opt(SCREEN_W / 4, 25, c0, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Stopwatch", -1);
    dtext_opt(3 * SCREEN_W / 4, 25, c1, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Timer", -1);
    if (active == 0) drect(0, 46, SCREEN_W / 2, 50, CP_TAB_HL);
    else drect(SCREEN_W / 2, 46, SCREEN_W, 50, CP_TAB_HL);
    dhline(50, C_RGB(10, 10, 10));
}

void update_stopwatch(int tx, int ty, uint32_t now) {
    uint32_t current_ms = state.sw_acc;
    if (state.sw_running) current_ms += (now - state.sw_start);

    if (ty >= 180 && ty <= 230) {
        if (tx >= 20 && tx <= 150) {
            set_visual_press(1, now);
            if (state.sw_running) { state.sw_acc += (now - state.sw_start); state.sw_running = false; }
            else { state.sw_start = now; state.sw_running = true; }
        } else if (tx >= 170 && tx <= 300) {
            set_visual_press(2, now);
            if (state.sw_running) {
                if (state.laps_cnt < 30) {
                    uint32_t last_tot = (state.laps_cnt > 0) ? laps[state.laps_cnt - 1].total : 0;
                    laps[state.laps_cnt] = { current_ms - last_tot, current_ms };
                    state.laps_cnt++;
                }
            } else if (current_ms > 0) { state.sw_acc = 0; state.laps_cnt = 0; }
        }
    }

    drect(10, 60, 310, 160, C_BLACK);
    ::drect_border(10, 60, 310, 160, (int)C_NONE, 2, (int)C_RGB(10, 10, 10));

    int csec = (current_ms / 10) % 100, sec = (current_ms / 1000) % 60, min = (current_ms / 60000) % 60;
    draw_digit(20, 80, 35, 70, 8, min / 10, CP_LCD_ON, CP_LCD_OFF);
    draw_digit(65, 80, 35, 70, 8, min % 10, CP_LCD_ON, CP_LCD_OFF);
    draw_colon(105, 80, 6, 70, CP_LCD_ON);
    draw_digit(115, 80, 35, 70, 8, sec / 10, CP_LCD_ON, CP_LCD_OFF);
    draw_digit(160, 80, 35, 70, 8, sec % 10, CP_LCD_ON, CP_LCD_OFF);
    draw_colon(200, 80, 6, 70, CP_LCD_ON);
    draw_digit(215, 100, 25, 50, 6, csec / 10, CP_LCD_ON, CP_LCD_OFF);
    draw_digit(245, 100, 25, 50, 6, csec % 10, CP_LCD_ON, CP_LCD_OFF);

    draw_btn(20, 180, 130, 50, state.sw_running ? "Stop" : "Start", state.sw_running ? CP_BTN_RED : CP_BTN_GREEN, check_visual_press(1, now));
    draw_btn(170, 180, 130, 50, state.sw_running ? "Lap" : "Reset", CP_BTN_GREY, check_visual_press(2, now));

    int y = 250;
    for (int i = state.laps_cnt - 1; i >= 0; --i) {
        if (y > 480) break;
        char buf[32];
        sprintf(buf, "%02d", i + 1); dtext(20, y + 10, CP_LCD_ACCENT, buf);
        sprintf(buf, "%02d:%02d.%02d", (int)(laps[i].diff / 60000) % 60, (int)(laps[i].diff / 1000) % 60, (int)(laps[i].diff / 10) % 100); dtext(80, y + 10, C_WHITE, buf);
        sprintf(buf, "%02d:%02d.%02d", (int)(laps[i].total / 60000) % 60, (int)(laps[i].total / 1000) % 60, (int)(laps[i].total / 10) % 100); dtext(200, y + 10, C_WHITE, buf);
        dhline(y + 35, C_RGB(8, 8, 8)); y += 40;
    }
}

void update_timer(int tx, int ty, uint32_t now) {
    drect(5, 60, 315, 160, C_BLACK);
    ::drect_border(5, 60, 315, 160, (int)C_NONE, 2, (int)C_RGB(10, 10, 10));
    dtext_opt(50, 70, C_RGB(15, 15, 15), (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Hours", -1);
    dtext_opt(150, 70, C_RGB(15, 15, 15), (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Minutes", -1);
    dtext_opt(250, 70, C_RGB(15, 15, 15), (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "Seconds", -1);

    int digs[6];
    if (state.tm_mode == 0) { for (int i = 0; i < 6; ++i) digs[i] = tm_input[i]; }
    else {
        uint32_t r = state.tm_rem;
        int s = (r / 1000) % 60, m = (r / 60000) % 60, h = (r / 3600000);
        digs[0] = h / 10; digs[1] = h % 10; digs[2] = m / 10; digs[3] = m % 10; digs[4] = s / 10; digs[5] = s % 10;
    }
    draw_digit(10, 80, 35, 70, 8, digs[0], CP_LCD_ON, CP_LCD_OFF);
    draw_digit(55, 80, 35, 70, 8, digs[1], CP_LCD_ON, CP_LCD_OFF);
    draw_colon(95, 80, 6, 70, CP_LCD_ON);
    draw_digit(110, 80, 35, 70, 8, digs[2], CP_LCD_ON, CP_LCD_OFF);
    draw_digit(155, 80, 35, 70, 8, digs[3], CP_LCD_ON, CP_LCD_OFF);
    draw_colon(195, 80, 6, 70, CP_LCD_ON);
    draw_digit(210, 80, 35, 70, 8, digs[4], CP_LCD_ON, CP_LCD_OFF);
    draw_digit(255, 80, 35, 70, 8, digs[5], CP_LCD_ON, CP_LCD_OFF);

    if (state.tm_mode == 0) {
        if (ty >= 250) {
            int kx = (tx - 5) / 105, ky = (ty - 250) / 68;
            if (kx >= 0 && kx <= 2 && ky >= 0 && ky <= 3) {
                set_visual_press(100 + ky * 10 + kx, now);
                const char* key = KEYPAD[ky][kx];
                if (strcmp(key, "START") == 0) {
                    uint32_t ms = ((tm_input[0] * 10 + tm_input[1]) * 3600 + (tm_input[2] * 10 + tm_input[3]) * 60 + (tm_input[4] * 10 + tm_input[5])) * 1000;
                    if (ms > 0) { state.tm_rem = ms; state.tm_end = now + ms; state.tm_mode = 1; }
                } else if (strcmp(key, "DEL") == 0) { for (int i = 5; i > 0; --i) tm_input[i] = tm_input[i - 1]; tm_input[0] = 0; }
                else { for (int i = 0; i < 5; ++i) tm_input[i] = tm_input[i + 1]; tm_input[5] = key[0] - '0'; }
            }
        }
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 3; ++c) {
            const char* t = KEYPAD[r][c];
            draw_btn(5 + c * 105, 250 + r * 68, 100, 60, t, strcmp(t, "START") == 0 ? CP_BTN_GREEN : (strcmp(t, "DEL") == 0 ? CP_BTN_RED : C_RGB(8, 8, 8)), check_visual_press(100 + r * 10 + c, now));
        }
    } else {
        if (ty >= 180 && ty <= 230) {
            if (tx >= 20 && tx <= 150) { set_visual_press(3, now); state.tm_mode = 0; for (int i = 0; i < 6; ++i) tm_input[i] = 0; }
            else if (tx >= 170 && tx <= 300) {
                set_visual_press(4, now);
                if (state.tm_mode == 1) state.tm_mode = 2;
                else { state.tm_mode = 1; state.tm_end = now + state.tm_rem; }
            }
        }
        draw_btn(20, 180, 130, 50, "Cancel", CP_BTN_GREY, check_visual_press(3, now));
        draw_btn(170, 180, 130, 50, state.tm_mode == 2 ? "Resume" : "Pause", state.tm_mode == 2 ? CP_BTN_GREEN : C_RGB(31, 20, 0), check_visual_press(4, now));
    }
}

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif
    bool latched = false;
    while (true) {
        key_event_t ev = pollevent();
        int tx = -1, ty = -1;
        while (ev.type != KEYEV_NONE) {
            if (ev.type == KEYEV_TOUCH_DOWN && !latched) { latched = true; tx = ev.x; ty = ev.y; }
            else if (ev.type == KEYEV_TOUCH_UP) latched = false;
            else if (ev.type == KEYEV_DOWN && ev.key == KEY_EXIT) return 0;
            ev = pollevent();
        }
        uint32_t now = ticks_ms();
        if (state.tm_mode == 1) {
            if (now >= state.tm_end) { state.tm_mode = 3; state.tm_alert_start = now; state.tm_rem = 0; }
            else state.tm_rem = state.tm_end - now;
        }
        if (state.tm_mode == 3) {
            bool flash = ((now - state.tm_alert_start) / 500) % 2 == 0;
            dclear(flash ? C_WHITE : CP_BG);
            dtext_opt(SCREEN_W / 2, SCREEN_H / 2, flash ? C_BLACK : C_WHITE, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "TIME'S UP!", -1);
            if (tx >= 0 || ty >= 0) { state.tm_mode = 0; for (int i = 0; i < 6; ++i) tm_input[i] = 0; }
        } else {
            if (ty > 0 && ty < 50) { if (tx < SCREEN_W / 2) state.tab = 0; else state.tab = 1; }
            dclear(CP_BG); draw_tabs(state.tab);
            if (state.tab == 0) update_stopwatch(tx, ty, now); else update_timer(tx, ty, now);
        }
        dupdate(); sleep_ms(20);
    }
}
