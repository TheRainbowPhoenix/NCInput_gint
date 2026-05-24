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
#include <cstring>
#include <cstdlib>
#include "cinput.hpp"

using namespace cinput;

// =============================================================================
// CONFIGURATION
// =============================================================================

const int HEADER_H = 40;
const int LIST_ITEM_H = 45;
const int SECTION_H = 25;

const std::string THEME_NAME = "mint";
const double PI_CONST = 3.14159265358979323846;

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
        auto args = values;
        for (auto const& pair : CONSTANTS) args[pair.first] = pair.second;
        if (eq->formulas.count(target_var)) {
            double val = eq->formulas.at(target_var)(args);
            if (std::isnan(val) || std::isinf(val)) {
                error_str = "Math Error";
                result_str = "Error";
            } else {
                char buf[32];
                sprintf(buf, "%.5g %s", val, eq->units.count(target_var) ? eq->units.at(target_var).c_str() : "");
                result_str = buf;
                error_str = "";
            }
        } else {
            error_str = "No formula";
        }
    }

    void handle_input_click(const std::string& var_name) {
        std::string unit = eq->units.count(var_name) ? eq->units.at(var_name) : "";
        std::string prompt = "Enter " + var_name + " (" + unit + "):";
        InputResult res = input(prompt, "numeric_float", THEME_NAME, "qwerty", KEYEV_TOUCH_UP);
        if (res.success) {
            values[var_name] = std::atof(res.value.c_str());
            if (is_solvable()) solve();
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
                if (!highlight) {
                    ::drect_border(margin, inp_y, SCREEN_W - margin, inp_y + 35, (int)C_NONE, 1, (int)border);
                }
                color_t col = highlight ? t.txt_acc : t.txt;
                dtext_opt(20, inp_y + 17, col, (int)C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, val_buf, -1);
                if (std::isnan(values[var])) dtext_opt(SCREEN_W-20, inp_y + 17, t.txt_dim, (int)C_NONE, DTEXT_RIGHT, DTEXT_MIDDLE, "Tap to edit", -1);
            }
            y += 60;
        }

        if (y + 50 > HEADER_H && y < SCREEN_H) {
            bool ready = is_solvable();
            bool highlight = (focus_index == (int)input_vars.size() + 1);
            color_t fill_c = highlight ? t.accent : (ready ? t.key_spec : t.modal_bg);
            color_t txt_c = highlight ? t.txt_acc : (ready ? t.txt : t.txt_dim);
            drect(margin, y, SCREEN_W - margin, y + 40, fill_c);
            if (!highlight) ::drect_border(margin, y, SCREEN_W - margin, y + 40, (int)C_NONE, 1, (int)t.key_spec);
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

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif

    THEMES[THEME_NAME] = {
        C_RGB(28, 60, 28), C_RGB(28, 60, 28), C_RGB(31, 63, 31),
        C_RGB(16, 40, 16), C_RGB(0, 0, 0), C_RGB(2, 12, 3),
        C_RGB(10, 28, 10), C_RGB(8, 40, 10), C_RGB(31, 63, 31),
        C_RGB(25, 58, 25), C_WHITE
    };

    // --- Mechanics ---
    Equation eq_kin_1("Velocity", {"v", "u", "a", "t"},
        {{"v", [](std::map<std::string, double>& x){ return x["u"] + x["a"]*x["t"]; }},
         {"u", [](std::map<std::string, double>& x){ return x["v"] - x["a"]*x["t"]; }},
         {"a", [](std::map<std::string, double>& x){ return (x["v"] - x["u"]) / x["t"]; }},
         {"t", [](std::map<std::string, double>& x){ return (x["v"] - x["u"]) / x["a"]; }}},
        {{"v","m/s"}, {"u","m/s"}, {"a","m/s^2"}, {"t","s"}});

    Equation eq_kin_2("Displacement", {"d", "u", "t", "a"},
        {{"d", [](std::map<std::string, double>& x){ return x["u"]*x["t"] + 0.5*x["a"]*std::pow(x["t"],2); }},
         {"u", [](std::map<std::string, double>& x){ return (x["d"] - 0.5*x["a"]*std::pow(x["t"],2))/x["t"]; }},
         {"a", [](std::map<std::string, double>& x){ return 2*(x["d"] - x["u"]*x["t"])/std::pow(x["t"],2); }}},
        {{"d","m"}, {"u","m/s"}, {"t","s"}, {"a","m/s^2"}});

    Equation eq_kin_3("Time-indep.", {"v", "u", "a", "d"},
        {{"v", [](std::map<std::string, double>& x){ return std::sqrt(std::pow(x["u"], 2) + 2*x["a"]*x["d"]); }},
         {"u", [](std::map<std::string, double>& x){ return std::sqrt(std::pow(x["v"], 2) - 2*x["a"]*x["d"]); }},
         {"a", [](std::map<std::string, double>& x){ return (std::pow(x["v"], 2) - std::pow(x["u"], 2)) / (2*x["d"]); }},
         {"d", [](std::map<std::string, double>& x){ return (std::pow(x["v"], 2) - std::pow(x["u"], 2)) / (2*x["a"]); }}},
        {{"v","m/s"}, {"u","m/s"}, {"a","m/s^2"}, {"d","m"}});

    Equation eq_newton("Newton's 2nd Law", {"F", "m", "a"},
        {{"F", [](std::map<std::string, double>& x){ return x["m"] * x["a"]; }},
         {"m", [](std::map<std::string, double>& x){ return x["F"] / x["a"]; }},
         {"a", [](std::map<std::string, double>& x){ return x["F"] / x["m"]; }}},
        {{"F","N"}, {"m","kg"}, {"a","m/s^2"}});

    Equation eq_ke("Kinetic Energy", {"K", "m", "v"},
        {{"K", [](std::map<std::string, double>& x){ return 0.5 * x["m"] * std::pow(x["v"], 2); }},
         {"m", [](std::map<std::string, double>& x){ return 2 * x["K"] / std::pow(x["v"], 2); }},
         {"v", [](std::map<std::string, double>& x){ return std::sqrt(2 * x["K"] / x["m"]); }}},
        {{"K","J"}, {"m","kg"}, {"v","m/s"}});

    Equation eq_pe("Potential Energy", {"U", "m", "g", "h"},
        {{"U", [](std::map<std::string, double>& x){ return x["m"] * x["g"] * x["h"]; }},
         {"m", [](std::map<std::string, double>& x){ return x["U"] / (x["g"] * x["h"]); }},
         {"h", [](std::map<std::string, double>& x){ return x["U"] / (x["m"] * x["g"]); }}},
        {{"U","J"}, {"m","kg"}, {"g","m/s^2"}, {"h","m"}});

    Equation eq_circ("Centripetal Force", {"F", "m", "v", "r"},
        {{"F", [](std::map<std::string, double>& x){ return (x["m"] * std::pow(x["v"], 2)) / x["r"]; }},
         {"m", [](std::map<std::string, double>& x){ return (x["F"] * x["r"]) / std::pow(x["v"], 2); }},
         {"r", [](std::map<std::string, double>& x){ return (x["m"] * std::pow(x["v"], 2)) / x["F"]; }},
         {"v", [](std::map<std::string, double>& x){ return std::sqrt((x["F"] * x["r"]) / x["m"]); }}},
        {{"F","N"}, {"m","kg"}, {"v","m/s"}, {"r","m"}});

    Equation eq_mom("Momentum (p=mv)", {"p", "m", "v"},
        {{"p", [](std::map<std::string, double>& x){ return x["m"] * x["v"]; }},
         {"m", [](std::map<std::string, double>& x){ return x["p"] / x["v"]; }},
         {"v", [](std::map<std::string, double>& x){ return x["p"] / x["m"]; }}},
        {{"p","kgm/s"}, {"m","kg"}, {"v","m/s"}});

    // --- Electricity ---
    Equation eq_ohm("Ohm's Law", {"V", "I", "R"},
        {{"V", [](std::map<std::string, double>& x){ return x["I"] * x["R"]; }},
         {"I", [](std::map<std::string, double>& x){ return x["V"] / x["R"]; }},
         {"R", [](std::map<std::string, double>& x){ return x["V"] / x["I"]; }}},
        {{"V","V"}, {"I","A"}, {"R","ohm"}});

    Equation eq_elec_p("Power (P=VI)", {"P", "V", "I"},
        {{"P", [](std::map<std::string, double>& x){ return x["V"] * x["I"]; }},
         {"V", [](std::map<std::string, double>& x){ return x["P"] / x["I"]; }},
         {"I", [](std::map<std::string, double>& x){ return x["P"] / x["V"]; }}},
        {{"P","W"}, {"V","V"}, {"I","A"}});

    Equation eq_coulomb("Coulomb's Law", {"F", "q1", "q2", "r", "k"},
        {{"F", [](std::map<std::string, double>& x){ return x["k"] * std::abs(x["q1"]*x["q2"]) / std::pow(x["r"], 2); }},
         {"r", [](std::map<std::string, double>& x){ return std::sqrt(x["k"] * std::abs(x["q1"]*x["q2"]) / x["F"]); }}},
        {{"F","N"}, {"q1","C"}, {"q2","C"}, {"r","m"}, {"k","const"}});

    Equation eq_cap("Capacitor (Q=CV)", {"Q", "C", "V"},
        {{"Q", [](std::map<std::string, double>& x){ return x["C"] * x["V"]; }},
         {"C", [](std::map<std::string, double>& x){ return x["Q"] / x["V"]; }},
         {"V", [](std::map<std::string, double>& x){ return x["Q"] / x["C"]; }}},
        {{"Q","C"}, {"C","F"}, {"V","V"}});

    // --- Waves & Light ---
    Equation eq_wave("Wave Eq (v=fL)", {"v", "f", "L"},
        {{"v", [](std::map<std::string, double>& x){ return x["f"] * x["L"]; }},
         {"f", [](std::map<std::string, double>& x){ return x["v"] / x["L"]; }},
         {"L", [](std::map<std::string, double>& x){ return x["v"] / x["f"]; }}},
        {{"v","m/s"}, {"f","Hz"}, {"L","m"}});

    Equation eq_period("Period (T=1/f)", {"T", "f"},
        {{"T", [](std::map<std::string, double>& x){ return 1.0 / x["f"]; }},
         {"f", [](std::map<std::string, double>& x){ return 1.0 / x["T"]; }}},
        {{"T","s"}, {"f","Hz"}});

    Equation eq_snell("Snell's Law", {"n1", "n2", "th1", "th2"},
        {{"n2", [](std::map<std::string, double>& x){ return x["n1"] * std::sin(x["th1"] * PI_CONST / 180.0) / std::sin(x["th2"] * PI_CONST / 180.0); }},
         {"n1", [](std::map<std::string, double>& x){ return x["n2"] * std::sin(x["th2"] * PI_CONST / 180.0) / std::sin(x["th1"] * PI_CONST / 180.0); }}},
        {{"th1","deg"}, {"th2","deg"}});

    // --- Chemistry ---
    Equation eq_moles("Molar Mass", {"n", "mass", "M"},
        {{"n", [](std::map<std::string, double>& x){ return x["mass"] / x["M"]; }},
         {"mass", [](std::map<std::string, double>& x){ return x["n"] * x["M"]; }},
         {"M", [](std::map<std::string, double>& x){ return x["mass"] / x["n"]; }}},
        {{"n","mol"}, {"mass","g"}, {"M","g/mol"}});

    Equation eq_molarity("Molarity (C=n/V)", {"C", "n", "V"},
        {{"C", [](std::map<std::string, double>& x){ return x["n"] / x["V"]; }},
         {"n", [](std::map<std::string, double>& x){ return x["C"] * x["V"]; }},
         {"V", [](std::map<std::string, double>& x){ return x["n"] / x["C"]; }}},
        {{"C","M"}, {"n","mol"}, {"V","L"}});

    Equation eq_gas("Ideal Gas (PV=nRT)", {"P", "V", "n", "T", "R"},
        {{"P", [](std::map<std::string, double>& x){ return (x["n"] * x["R"] * x["T"]) / x["V"]; }},
         {"V", [](std::map<std::string, double>& x){ return (x["n"] * x["R"] * x["T"]) / x["P"]; }},
         {"n", [](std::map<std::string, double>& x){ return (x["P"] * x["V"]) / (x["R"] * x["T"]); }},
         {"T", [](std::map<std::string, double>& x){ return (x["P"] * x["V"]) / (x["n"] * x["R"]); }}},
        {{"P","Pa"}, {"V","m^3"}, {"n","mol"}, {"T","K"}, {"R","const"}});

    Equation eq_density("Density (D=m/V)", {"D", "m", "V"},
        {{"D", [](std::map<std::string, double>& x){ return x["m"] / x["V"]; }},
         {"m", [](std::map<std::string, double>& x){ return x["D"] * x["V"]; }},
         {"V", [](std::map<std::string, double>& x){ return x["m"] / x["D"]; }}},
        {{"D","g/mL"}, {"m","g"}, {"V","mL"}});

    Equation eq_heat("Heat (q=mcDT)", {"q", "m", "c", "DT"},
        {{"q", [](std::map<std::string, double>& x){ return x["m"] * x["c"] * x["DT"]; }},
         {"m", [](std::map<std::string, double>& x){ return x["q"] / (x["c"] * x["DT"]); }}},
        {{"q","J"}, {"m","g"}, {"c","J/gC"}, {"DT","C"}});

    std::vector<std::string> categories = {"Mechanics", "Electricity", "Waves & Light", "Chemistry"};
    std::map<std::string, std::map<std::string, std::vector<Equation*>>> menu_tree;

    menu_tree["Mechanics"]["1-D Kinematics"] = {&eq_kin_1, &eq_kin_2, &eq_kin_3};
    menu_tree["Mechanics"]["Newton's Laws"] = {&eq_newton};
    menu_tree["Mechanics"]["Work & Energy"] = {&eq_ke, &eq_pe};
    menu_tree["Mechanics"]["Circular Motion"] = {&eq_circ};
    menu_tree["Mechanics"]["Momentum"] = {&eq_mom};

    menu_tree["Electricity"]["Circuits"] = {&eq_ohm, &eq_elec_p};
    menu_tree["Electricity"]["Electrostatics"] = {&eq_coulomb};
    menu_tree["Electricity"]["Capacitance"] = {&eq_cap};

    menu_tree["Waves & Light"]["Basics"] = {&eq_wave, &eq_period};
    menu_tree["Waves & Light"]["Optics"] = {&eq_snell};

    menu_tree["Chemistry"]["Stoichiometry"] = {&eq_moles, &eq_density};
    menu_tree["Chemistry"]["Solutions"] = {&eq_molarity};
    menu_tree["Chemistry"]["Gas Laws"] = {&eq_gas};
    menu_tree["Chemistry"]["Thermodynamics"] = {&eq_heat};

    enum class View { CATEGORIES, SECTIONS, EQUATIONS };
    View current_view = View::CATEGORIES;
    std::string current_cat = categories[0];
    std::string current_sec = "";

    while (true) {
        std::vector<ListItem> items;
        std::string title;

        if (current_view == View::CATEGORIES) {
            for (const auto& cat : categories) items.push_back({cat, "item", LIST_ITEM_H, false, true});
            title = "PhysChem";
        } else if (current_view == View::SECTIONS) {
            for (auto const& [sec, eqs] : menu_tree[current_cat]) {
                items.push_back({sec, "item", LIST_ITEM_H, false, true});
            }
            title = current_cat;
        } else {
            for (auto eq : menu_tree[current_cat][current_sec]) {
                items.push_back({eq->name, "item", LIST_ITEM_H, false, true});
            }
            title = current_sec;
        }

        ListView lv({0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H}, items, LIST_ITEM_H, THEME_NAME, SECTION_H);
        bool in_view = true;
        while (in_view) {
            lv.draw();
            const Theme& t = get_theme(THEME_NAME);
            drect(0, 0, SCREEN_W, HEADER_H, t.accent);

            if (current_view == View::CATEGORIES) {
                for (int i=0; i<3; ++i) drect(15, HEADER_H/2-8+i*5, 15+18, HEADER_H/2-8+i*5+1, t.txt_acc);
            } else {
                // Draw back arrow
                drect(10 + 2, 10 + 9, 10 + 18, 10 + 10, t.txt_acc);
                dline(10 + 2, 10 + 9, 10 + 9, 10 + 2, t.txt_acc);
                dline(10 + 2, 10 + 9, 10 + 9, 10 + 16, t.txt_acc);
            }

            dtext_opt(SCREEN_W/2, HEADER_H/2, t.txt_acc, (int)C_NONE, DTEXT_CENTER, DTEXT_MIDDLE, title.c_str(), -1);
            dupdate(); cleareventflips();

            if (keypressed(KEY_EXIT)) {
                if (current_view == View::EQUATIONS) { current_view = View::SECTIONS; in_view = false; }
                else if (current_view == View::SECTIONS) { current_view = View::CATEGORIES; in_view = false; }
                else return 0;
            }

            key_event_t ev = pollevent();
            std::vector<key_event_t> events;
            while(ev.type != KEYEV_NONE) {
                if (ev.type == KEYEV_TOUCH_UP && ev.y < HEADER_H && ev.x < 60) {
                    if (current_view == View::EQUATIONS) { current_view = View::SECTIONS; in_view = false; }
                    else if (current_view == View::SECTIONS) { current_view = View::CATEGORIES; in_view = false; }
                    else {
                        auto res = pick(categories, "Jump to...", THEME_NAME, false, KEYEV_TOUCH_UP);
                        if (res.success && !res.values.empty()) { current_cat = res.values[0]; current_view = View::SECTIONS; in_view = false; }
                    }
                }
                events.push_back(ev); ev = pollevent();
            }

            ListView::Action action;
            if (lv.update(events, action)) {
                if (action.type == "click") {
                    if (current_view == View::CATEGORIES) {
                        current_cat = categories[action.index];
                        current_view = View::SECTIONS;
                        in_view = false;
                    } else if (current_view == View::SECTIONS) {
                        // Get section name by iterating map at action.index
                        int i = 0;
                        for (auto const& [sec, eqs] : menu_tree[current_cat]) {
                            if (i == action.index) { current_sec = sec; break; }
                            i++;
                        }
                        current_view = View::EQUATIONS;
                        in_view = false;
                    } else {
                        SolverActivity solver(menu_tree[current_cat][current_sec][action.index]);
                        solver.run();
                    }
                }
            }
            sleep_ms(10);
        }
    }
}
