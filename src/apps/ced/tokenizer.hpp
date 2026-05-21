#pragma once
#include <string>
#include <vector>
#include <gint/display.h>

namespace ced {

class Tokenizer {
public:
    Tokenizer(const std::string& line, const std::string& theme_name);
    bool next(std::string& text, color_t& color);

private:
    const std::string& line;
    size_t i;
    size_t length;
    color_t col_txt, col_kw, col_str, col_com, col_num, col_op;
};

} // namespace ced
