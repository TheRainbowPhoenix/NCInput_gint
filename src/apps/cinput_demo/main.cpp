#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <string>
#include <vector>
#include <cstdio>
#include "cinput.hpp"
#include "cdateinput.hpp"

using namespace cinput;

const int HEADER_H = 40;

void draw_header(const std::string& theme_name, const std::string& title = "Demo App") {
    const Theme& t = get_theme(theme_name);
    // Header Bar
    drect(0, 0, SCREEN_W, HEADER_H, t.accent);
    dtext_opt(SCREEN_W/2, HEADER_H/2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, title.c_str(), -1);

    // Menu Icon (Hamburger)
    color_t col = t.txt_acc;
    int x = 10, y = 10;
    for (int i = 0; i < 3; ++i) {
        drect(x, y + 4 + i*5, x + 18, y + 5 + i*5, col);
    }
}

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif

    std::vector<std::string> themes = {"light", "dark", "grey"};
    std::vector<std::string> layouts = {"qwerty", "azerty", "qwertz", "abc"};

    int curr_theme_idx = 0;
    int curr_layout_idx = 0;

    bool running = true;
    bool touch_latched = false;

    while (running) {
        std::string current_theme_name = themes[curr_theme_idx];
        std::string current_layout_name = layouts[curr_layout_idx];
        const Theme& t = get_theme(current_theme_name);

        // Draw Main Shell
        dclear(t.modal_bg);
        draw_header(current_theme_name);

        // Main Content
        const char* msg = "Tap Menu or press [MENU]";
        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, msg, -1);
        std::string theme_msg = "Theme: " + current_theme_name;
        dtext_opt(SCREEN_W/2, SCREEN_H/2 + 20, t.key_spec, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, theme_msg.c_str(), -1);

        dupdate();

        // Input Loop
        cleareventflips();

        bool open_menu = false;

        // Physical Key
        if (keypressed(KEY_MENU) || keypressed(KEY_F1)) {
            open_menu = true;
        }
        if (keypressed(KEY_EXIT)) {
            running = false;
        }

        // Event Processing
        key_event_t e = pollevent();
        std::vector<key_event_t> events;
        while (e.type != KEYEV_NONE) {
            events.push_back(e);
            e = pollevent();
        }

        for (const auto& ev : events) {
            if (ev.type == KEYEV_TOUCH_UP) {
                touch_latched = false;
            } else if (ev.type == KEYEV_TOUCH_DOWN && !touch_latched) {
                touch_latched = true;
                // Check Header Click on Press
                if (ev.y < HEADER_H && ev.x < 50) {
                    open_menu = true;
                }
            }
        }

        // Handle Menu Action
        if (open_menu) {
            cleareventflips();
            std::vector<std::string> opts = {
                "Text Input Demo",
                "Integer Input Demo",
                "Float Input Demo",
                "List Picker Demo",
                "Date Picker Demo",
                "Time Picker Demo",
                "French Date Picker",
                "Confirmation Demo",
                "Switch Theme (" + themes[(curr_theme_idx+1)%themes.size()] + ")",
                "Switch Layout (" + layouts[(curr_layout_idx+1)%layouts.size()] + ")",
                "Quit"
            };

            PickResult choice_res = pick(opts, "App Menu", current_theme_name);

            if (choice_res.success && !choice_res.values.empty()) {
                std::string choice = choice_res.values[0];

                if (choice == "Quit") {
                    running = false;
                } else if (choice.find("Switch Theme") != std::string::npos) {
                    curr_theme_idx = (curr_theme_idx + 1) % themes.size();
                } else if (choice.find("Switch Layout") != std::string::npos) {
                    curr_layout_idx = (curr_layout_idx + 1) % layouts.size();
                } else if (choice == "Text Input Demo") {
                    InputResult res = input("Enter text (" + current_layout_name + "):", "text", current_theme_name, current_layout_name);
                    if (res.success) {
                        dclear(t.modal_bg);
                        std::string typed = "You typed: " + res.value;
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, typed.c_str(), -1);
                        dupdate();
                        sleep_ms(2000);
                    }
                } else if (choice == "Integer Input Demo") {
                    InputResult res = input("Enter Integer:", "numeric_int negative", current_theme_name);
                    if (res.success) {
                        dclear(t.modal_bg);
                        std::string val = "Value: " + res.value;
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, val.c_str(), -1);
                        dupdate();
                        sleep_ms(2000);
                    }
                } else if (choice == "Float Input Demo") {
                    InputResult res = input("Enter Float:", "numeric_float", current_theme_name);
                    if (res.success) {
                        dclear(t.modal_bg);
                        std::string val = "Value: " + res.value;
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, val.c_str(), -1);
                        dupdate();
                        sleep_ms(2000);
                    }
                } else if (choice == "List Picker Demo") {
                    std::vector<std::string> demo_opts;
                    for (int i = 1; i <= 20; ++i) demo_opts.push_back("Item " + std::to_string(i));
                    PickResult res = pick(demo_opts, "Multi Select Demo", current_theme_name, true);
                    if (res.success) {
                        dclear(t.modal_bg);
                        std::string res_str = "Selected: ";
                        for (size_t i = 0; i < res.values.size(); ++i) {
                            res_str += res.values[i] + (i == res.values.size() - 1 ? "" : ", ");
                        }
                        dtext_opt(10, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, res_str.c_str(), SCREEN_W-20);
                        dupdate();
                        sleep_ms(2000);
                    }
                } else if (choice == "Date Picker Demo") {
                    cdateinput::Date* res = cdateinput::datepick("Select Date", 2026, 5, 15, current_theme_name);
                    if (res) {
                        dclear(t.modal_bg);
                        char buf[64]; sprintf(buf, "Selected: %04d-%02d-%02d", res->year, res->month, res->day);
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
                        dupdate(); delete res;
                        sleep_ms(2000);
                    }
                } else if (choice == "Time Picker Demo") {
                    cdateinput::Time* res = cdateinput::timepick("Select Time", 12, 30, current_theme_name);
                    if (res) {
                        dclear(t.modal_bg);
                        char buf[64]; sprintf(buf, "Selected: %02d:%02d", res->hour, res->minute);
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
                        dupdate(); delete res;
                        sleep_ms(2000);
                    }
                } else if (choice == "French Date Picker") {
                    std::vector<std::string> old_days = cdateinput::DAY_NAMES;
                    std::vector<std::string> old_dows = cdateinput::DOW_SINGLE;
                    std::vector<std::string> old_months = cdateinput::MONTH_NAMES;
                    std::vector<std::string> old_months_long = cdateinput::MONTH_NAMES_LONG;

                    cdateinput::DAY_NAMES = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
                    cdateinput::DOW_SINGLE = {"D", "L", "M", "M", "J", "V", "S"};
                    cdateinput::MONTH_NAMES = {"", "Jan", "Fev", "Mar", "Avr", "Mai", "Jun", "Jul", "Aoû", "Sep", "Oct", "Nov", "Dec"};
                    cdateinput::MONTH_NAMES_LONG = {"", "Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Decembre"};

                    cdateinput::Date* res = cdateinput::datepick("Date de naissance", 2000, 1, 1, current_theme_name);

                    cdateinput::DAY_NAMES = old_days;
                    cdateinput::DOW_SINGLE = old_dows;
                    cdateinput::MONTH_NAMES = old_months;
                    cdateinput::MONTH_NAMES_LONG = old_months_long;

                    if (res) {
                        dclear(t.modal_bg);
                        char buf[64]; sprintf(buf, "Choisi: %04d-%02d-%02d", res->year, res->month, res->day);
                        dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, buf, -1);
                        dupdate(); delete res;
                        sleep_ms(2000);
                    }
                } else if (choice == "Confirmation Demo") {
                    bool res = ask("Confirmation", "Are you sure?", "Yes", "No", current_theme_name);
                    dclear(t.modal_bg);
                    const char* res_txt = res ? "You clicked Yes" : "You clicked No";
                    dtext_opt(SCREEN_W/2, SCREEN_H/2, t.txt, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, res_txt, -1);
                    dupdate();
                    sleep_ms(2000);
                }
            }
        }

        sleep_ms(10);
    }

    return 0;
}
