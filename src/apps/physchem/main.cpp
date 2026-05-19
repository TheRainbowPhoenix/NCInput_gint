#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "cinput.hpp"

using namespace cinput;

// =============================================================================
// CONFIGURATION
// =============================================================================

const int HEADER_H = 40;
const int LIST_ITEM_H = 45;
const int SECTION_H = 25;

const std::string THEME_NAME = "mint";

// =============================================================================
// EQUATION DATA
// =============================================================================

struct Equation {
    std::string name;
    std::vector<std::string> variables;
    std::map<std::string, std::function<double(std::map<std::string, double>&)>> formulas;
    std::map<std::string, std::string> units;

    Equation(std::string n, std::vector<std::string> v,
             std::map<std::string, std::function<double(std::map<std::string, double>&)>> f,
             std::map<std::string, std::string> u = {})
        : name(n), variables(v), formulas(f), units(u) {}
};

static std::map<std::string, double> CONSTANTS = {
    {"g", 9.81}, {"R", 8.314}, {"c", 3.0e8}, {"k", 9.0e9}, {"G", 6.674e-11}
};

// =============================================================================
// SOLVER ACTIVITY
// =============================================================================

class SolverActivity {
public:
    SolverActivity(Equation* equation) : eq(equation) {
        target_var = equation->variables[0];
        for (const auto& v : equation->variables) {
            if (CONSTANTS.count(v)) values[v] = CONSTANTS[v];
            else values[v] = NAN;
        }
        scroll_y = 0;
        focus_index = 0;
        result_str = "---";
        recalc_layout();
    }

    void recalc_layout() {
        input_vars.clear();
        for (const auto& v : eq->variables) {
            if (v != target_var && CONSTANTS.find(v) == CONSTANTS.end()) {
                input_vars.push_back(v);
            }
        }
        int content_h = 60 + (input_vars.size() * 60) + 60 + 80;
        int view_h = SCREEN_H - HEADER_H;
        max_scroll = std::max(0, content_h - view_h);
        result_str = "---";
        error_str = "";
    }

    bool is_solvable() {
        for (const auto& v : input_vars) {
            if (std::isnan(values[v])) return false;
        }
        return true;
    }

    void solve() {
        if (!is_solvable()) { error_str = "Fill all fields"; return; }
        try {
            auto args = values;
            for (auto const& [k, v] : CONSTANTS) args[k] = v;
            if (eq->formulas.count(target_var)) {
                double val = eq->formulas.at(target_var)(args);
                char buf[32];
                sprintf(buf, "%.5g %s", val, eq->units.count(target_var) ? eq->units.at(target_var).c_str() : "");
                result_str = buf;
                error_str = "";
            } else {
                error_str = "No formula";
            }
        } catch (...) {
            error_str = "Math Error";
            result_str = "Error";
        }
    }

    void handle_input_click(const std::string& var_name) {
        std::string unit = eq->units.count(var_name) ? eq->units.at(var_name) : "";
        std::string prompt = "Enter " + var_name + " (" + unit + "):";
        InputResult res = input(prompt, "numeric_float", THEME_NAME, "qwerty", KEYEV_TOUCH_UP);
        if (res.success) {
            try {
                values[var_name] = std::stod(res.value);
                if (is_solvable()) solve();
            } catch (...) {}
        }
    }

    void cycle_target() {
        std::vector<std::string> opts;
        for (const auto& v : eq->variables) {
            if (eq->formulas.count(v)) opts.push_back(v);
        }
        PickResult res = pick(opts, "Solve for...", THEME_NAME, false, KEYEV_TOUCH_UP);
        if (res.success && !res.values.empty()) {
            target_var = res.values[0];
            recalc_layout();
        }
    }

    void draw_back_arrow(int x, int y, color_t col) {
        drect(x + 2, y + 9, x + 18, y + 10, col);
        dline(x + 2, y + 9, x + 9, y + 2, col);
        dline(x + 2, y + 10, x + 9, y + 3, col);
        dline(x + 2, y + 9, x + 9, y + 16, col);
        dline(x + 2, y + 10, x + 9, y + 17, col);
    }

    void draw() {
        const Theme& t = get_theme(THEME_NAME);
        dclear(t.modal_bg);
        int top_pad = 12;
        int y = HEADER_H + top_pad - scroll_y;
        int margin = 10;

        if (y + 50 > HEADER_H && y < SCREEN_H) {
            dtext_opt(margin, y, t.txt_dim, (int)C_NONE, DTEXT_LEFT, DTEXT_TOP, "Solving for:", -1);
            int btn_y = y + 15;
            bool highlight = (focus_index == 0);
            color_t bc = highlight ? t.accent : t.key_spec;
            color_t tc = highlight ? t.txt_acc : t.txt;
            drect(margin, btn_y, SCREEN_W - margin, btn_y + 35, bc);
            std::string label = target_var + " (" + (eq->units.count(target_var) ? eq->units.at(target_var) : "") + ")";
            dtext_opt(SCREEN_W/2, btn_y + 17, tc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, label.c_str(), -1);
            dtext_opt(SCREEN_W-20, btn_y + 17, tc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "v", -1);
        }
        y += 60;

        for (size_t i = 0; i < input_vars.size(); ++i) {
            if (y + 50 > HEADER_H && y < SCREEN_H) {
                std::string var = input_vars[i];
                bool highlight = (focus_index == (int)i + 1);
                std::string unit = eq->units.count(var) ? eq->units.at(var) : "";
                dtext_opt(margin, y, t.txt_dim, (int)C_NONE, DTEXT_LEFT, DTEXT_TOP, (var + " (" + unit + ")").c_str(), -1);
                int inp_y = y + 15;
                color_t bc = highlight ? t.accent : t.modal_bg;
                color_t border = highlight ? t.accent : t.key_spec;
                char val_buf[32];
                if (std::isnan(values[var])) strcpy(val_buf, "");
                else sprintf(val_buf, "%.5g", values[var]);
                drect(margin, inp_y, SCREEN_W - margin, inp_y + 35, bc);
                if (!highlight) ::drect_border(margin, inp_y, SCREEN_W - margin, inp_y + 35, (int)C_NONE, 1, (int)border);
                color_t col = highlight ? t.txt_acc : t.txt;
                dtext_opt(20, inp_y + 17, col, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, val_buf, -1);
                if (std::isnan(values[var])) dtext_opt(SCREEN_W-20, inp_y + 17, t.txt_dim, (int)C_NONE, DTEXT_RIGHT, DTEXT_MIDDLE, "Tap to edit", -1);
            }
            y += 60;
        }

        if (y + 50 > HEADER_H && y < SCREEN_H) {
            bool ready = is_solvable();
            bool highlight = (focus_index == (int)input_vars.size() + 1);
            color_t fill_c = ready ? t.accent : t.key_spec;
            color_t txt_c = ready ? t.txt_acc : t.txt_dim;
            drect(margin, y, SCREEN_W - margin, y + 40, fill_c);
            dtext_opt(SCREEN_W/2, y + 20, txt_c, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, "SOLVE", -1);
        }
        y += 60;

        if (y + 80 > HEADER_H && y < SCREEN_H) {
            dhline(y, t.key_spec);
            dtext_opt(SCREEN_W/2, y + 20, t.txt_dim, (int)C_NONE, DTEXT_CENTER, DTEXT_TOP, "Result", -1);
            color_t res_col = !error_str.empty() ? C_RGB(31, 0, 0) : t.txt;
            std::string disp = !error_str.empty() ? error_str : result_str;
            dtext_opt(SCREEN_W/2, y + 50, res_col, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, disp.c_str(), -1);
        }

        drect(0, 0, SCREEN_W, HEADER_H, t.accent);
        draw_back_arrow(10, 10, t.txt_acc);
        dtext_opt(SCREEN_W/2, HEADER_H/2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, eq->name.c_str(), -1);
    }

    void run() {
        bool running = true;
        int touch_start_y = 0;
        bool touch_latched = false;
        bool is_dragging = false;

        while (running) {
            draw(); dupdate(); cleareventflips();
            if (keypressed(KEY_DEL) || keypressed(KEY_EXIT)) running = false;
            if (keypressed(KEY_UP)) focus_index = std::max(0, focus_index - 1);
            if (keypressed(KEY_DOWN)) focus_index = std::min((int)input_vars.size() + 1, focus_index + 1);
            if (keypressed(KEY_EXE)) {
                if (focus_index == 0) cycle_target();
                else if (focus_index <= (int)input_vars.size()) handle_input_click(input_vars[focus_index - 1]);
                else solve();
            }

            key_event_t ev = pollevent();
            while (ev.type != KEYEV_NONE) {
                if (ev.type == KEYEV_TOUCH_DOWN) {
                    touch_latched = true; touch_start_y = ev.y; is_dragging = false;
                } else if (ev.type == KEYEV_TOUCH_DRAG && touch_latched) {
                    if (std::abs(ev.y - touch_start_y) > 10) is_dragging = true;
                    if (is_dragging) {
                        scroll_y = std::max(0, std::min(max_scroll, scroll_y - (ev.y - touch_start_y)));
                        touch_start_y = ev.y;
                    }
                } else if (ev.type == KEYEV_TOUCH_UP && touch_latched) {
                    touch_latched = false;
                    if (!is_dragging) {
                        if (ev.y < HEADER_H && ev.x < 80) running = false;
                        else if (ev.y > HEADER_H) {
                            int abs_y = ev.y - HEADER_H + scroll_y;
                            if (abs_y < 60) { focus_index = 0; cycle_target(); }
                            else if (abs_y < 60 + (int)input_vars.size() * 60) {
                                int idx = (abs_y - 60) / 60;
                                focus_index = idx + 1; handle_input_click(input_vars[idx]);
                            } else if (abs_y >= 60 + (int)input_vars.size() * 60) {
                                focus_index = input_vars.size() + 1; solve();
                            }
                        }
                    }
                }
                ev = pollevent();
            }
            sleep_ms(10);
        }
    }

private:
    Equation* eq;
    std::string target_var;
    std::map<std::string, double> values;
    std::vector<std::string> input_vars;
    int scroll_y, max_scroll, focus_index;
    std::string result_str, error_str;
};

// =============================================================================
// MAIN APP
// =============================================================================

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    extern "C" void simulator_init();
    simulator_init();
#endif

    THEMES[THEME_NAME] = {
        C_RGB(29, 31, 29), C_RGB(29, 31, 29), C_RGB(31, 31, 31),
        C_RGB(22, 26, 22), C_RGB(0, 0, 0), C_RGB(2, 6, 3),
        C_RGB(10, 14, 10), C_RGB(4, 18, 12), C_RGB(31, 31, 31),
        C_RGB(20, 26, 20), C_WHITE
    };

    Equation v_eq("Velocity", {"v", "u", "a", "t"},
        {{"v", [](auto& x){ return x["u"] + x["a"]*x["t"]; }},
         {"u", [](auto& x){ return x["v"] - x["a"]*x["t"]; }},
         {"a", [](auto& x){ return (x["v"] - x["u"]) / x["t"]; }},
         {"t", [](auto& x){ return (x["v"] - x["u"]) / x["a"]; }}},
        {{"v","m/s"}, {"u","m/s"}, {"a","m/s^2"}, {"t","s"}});

    Equation d_eq("Displacement", {"d", "u", "t", "a"},
        {{"d", [](auto& x){ return x["u"]*x["t"] + 0.5*x["a"]*std::pow(x["t"],2); }},
         {"u", [](auto& x){ return (x["d"] - 0.5*x["a"]*std::pow(x["t"],2))/x["t"]; }},
         {"a", [](auto& x){ return 2*(x["d"] - x["u"]*x["t"])/std::pow(x["t"],2); }}},
        {{"d","m"}, {"u","m/s"}, {"t","s"}, {"a","m/s^2"}});

    std::map<std::string, std::vector<Equation*>> menu_tree = {
        {"Mechanics", {&v_eq, &d_eq}}
    };

    std::vector<std::string> categories;
    for (auto const& [k, v] : menu_tree) categories.push_back(k);
    std::string current_cat = categories[0];

    while (true) {
        std::vector<ListItem> items;
        for (auto eq : menu_tree[current_cat]) {
            items.push_back({eq->name, "item", LIST_ITEM_H, false, true});
        }

        ListView lv({0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H}, items, LIST_ITEM_H, THEME_NAME, SECTION_H);
        bool in_main = true;
        while (in_main) {
            lv.draw();
            const Theme& t = get_theme(THEME_NAME);
            drect(0, 0, SCREEN_W, HEADER_H, t.accent);
            for (int i=0; i<3; ++i) drect(15, HEADER_H/2-8+i*5, 15+18, HEADER_H/2-8+i*5+1, t.txt_acc);
            dtext_opt(SCREEN_W/2, HEADER_H/2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, current_cat.c_str(), -1);
            dupdate(); cleareventflips();

            if (keypressed(KEY_MENU)) {
                auto res = pick(categories, "Category", THEME_NAME, false, KEYEV_TOUCH_UP);
                if (res.success && !res.values.empty()) { current_cat = res.values[0]; in_main = false; }
            }
            if (keypressed(KEY_EXIT)) return 0;

            key_event_t ev = pollevent();
            std::vector<key_event_t> events;
            while(ev.type != KEYEV_NONE) {
                if (ev.type == KEYEV_TOUCH_UP && ev.y < HEADER_H && ev.x < 60) {
                    auto res = pick(categories, "Category", THEME_NAME, false, KEYEV_TOUCH_UP);
                    if (res.success && !res.values.empty()) { current_cat = res.values[0]; in_main = false; }
                }
                events.push_back(ev); ev = pollevent();
            }

            ListView::Action action;
            if (lv.update(events, action)) {
                if (action.type == "click") {
                    SolverActivity solver(menu_tree[current_cat][action.index]);
                    solver.run();
                }
            }
            sleep_ms(10);
        }
    }
}
