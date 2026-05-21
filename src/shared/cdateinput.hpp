#ifndef CDATEINPUT_HPP
#define CDATEINPUT_HPP

#include <string>
#include <vector>
#include "cinput.hpp"

namespace cdateinput {

// =============================================================================
// CONSTANTS
// =============================================================================

extern std::vector<std::string> DAY_NAMES;
extern std::vector<std::string> DOW_SINGLE;
extern std::vector<std::string> MONTH_NAMES;
extern std::vector<std::string> MONTH_NAMES_LONG;

// =============================================================================
// DATE MATH UTILITIES
// =============================================================================

bool is_leap(int year);
int days_in_month(int year, int month);
int day_of_week(int year, int month, int day);
void add_days(int& y, int& m, int& d, int delta);

// =============================================================================
// DATE PICKER WIDGET
// =============================================================================

struct Date {
    int year;
    int month;
    int day;
    bool operator<(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
    bool operator>(const Date& other) const { return other < *this; }
};

class DatePicker {
public:
    DatePicker(const std::string& prompt = "Select Date", int default_y = 2026, int default_m = 1, int default_d = 1,
               const std::string& theme = "light", Date* min_date = nullptr, Date* max_date = nullptr);

    void draw_bold_text(int x, int y, color_t fg, const std::string& text);
    void draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed);
    void draw();
    Date* run();

private:
    Date clamp_date(int y, int m, int d);

    std::string m_prompt;
    Date* m_min_date;
    Date* m_max_date;
    Date m_current;

    std::string m_theme_name;
    const cinput::Theme& m_theme;

    bool btn_ok_pressed;
    bool btn_cn_pressed;
    bool left_arrow_pressed;
    bool right_arrow_pressed;

    std::string num_buffer;

    int header_h;
    int footer_h;
    int btn_w;
};

// =============================================================================
// TIME PICKER WIDGET
// =============================================================================

struct Time {
    int hour;
    int minute;
};

class TimePicker {
public:
    TimePicker(const std::string& prompt = "Select Time", int default_h = 12, int default_m = 0,
               const std::string& theme = "light", Time* min_time = nullptr, Time* max_time = nullptr);

    void enforce_boundaries();
    void draw_bold_text(int x, int y, color_t fg, const std::string& text);
    void draw_btn(int x, int y, int w, int h, const std::string& text, bool pressed);
    void draw();
    Time* run();

private:
    std::string m_prompt;
    Time* m_min_time;
    Time* m_max_time;

    float offset_h;
    float offset_m;

    const cinput::Theme& m_theme;
    std::string num_buffer;

    bool btn_ok_pressed;
    bool btn_cn_pressed;

    int header_h;
    int footer_h;
    int item_h;
    int btn_w;
    int center_y;
    int focus_col;
};

// =============================================================================
// PUBLIC EXPORTED FUNCTIONS
// =============================================================================

Date* datepick(const std::string& prompt = "Select Date", int default_y = 2026, int default_m = 1, int default_d = 1,
              const std::string& theme = "light", Date* min_date = nullptr, Date* max_date = nullptr);

Time* timepick(const std::string& prompt = "Select Time", int default_h = 12, int default_m = 0,
              const std::string& theme = "light", Time* min_time = nullptr, Time* max_time = nullptr);

} // namespace cdateinput

#endif // CDATEINPUT_HPP
