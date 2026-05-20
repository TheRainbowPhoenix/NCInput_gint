#include "tokenizer.hpp"
#include <set>
#include <algorithm>
#include "cinput.hpp"

namespace ced {

static const std::set<std::string> KEYWORDS = {
    "def", "class", "if", "else", "elif", "while", "for", "import", "from",
    "return", "True", "False", "None", "break", "continue", "pass", "try",
    "except", "with", "as", "global", "print", "len", "range", "in", "is",
    "not", "and", "or"
};

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

Tokenizer::Tokenizer(const std::string& line, const std::string& theme_name) : line(line) {
    i = 0;
    length = line.length();
    col_txt = cinput::get_theme(theme_name).txt;
    if (theme_name == "dark") {
        col_kw  = 0x4C7F; // Light Blue
        col_str = 0x8666; // Light Green
        col_com = 0x7BEF; // Gray
        col_num = C_RED;
        col_op  = 0xF81F; // Magenta
    } else {
        col_kw  = C_BLUE;
        col_str = 0x0480; // Dark Green
        col_com = 0x7BEF; // Gray
        col_num = C_RED;
        col_op  = 0xF81F; // Magenta
    }
}

bool Tokenizer::next(std::string& text, color_t& color) {
    if (i >= length) return false;

    const std::string operators = "+-*/%=<>!&|^~";
    const std::string separators = "()[]{}:,.";
    char c = line[i];

    if (c == '#') {
        text = line.substr(i);
        color = col_com;
        i = length;
        return true;
    } else if (c == '"' || c == '\'') {
        char quote = c;
        size_t start = i;
        i++;
        while (i < length && line[i] != quote) i++;
        if (i < length) i++;
        text = line.substr(start, i - start);
        color = col_str;
        return true;
    } else if (operators.find(c) != std::string::npos || separators.find(c) != std::string::npos) {
        text = std::string(1, c);
        color = col_op;
        i++;
        return true;
    } else if (c == ' ' || c == '\t') {
        text = std::string(1, c);
        color = col_txt;
        i++;
        return true;
    } else {
        size_t start = i;
        while (i < length) {
            char curr = line[i];
            if (operators.find(curr) != std::string::npos ||
                separators.find(curr) != std::string::npos ||
                curr == ' ' || curr == '\t' || curr == '#' ||
                curr == '"' || curr == '\'') {
                break;
            }
            i++;
        }
        text = line.substr(start, i - start);
        if (text.empty()) return false;
        if (is_digit(text[0])) color = col_num;
        else if (KEYWORDS.count(text)) color = col_kw;
        else color = col_txt;
        return true;
    }
}

} // namespace ced
