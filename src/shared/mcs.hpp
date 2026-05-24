#ifndef MCS_HPP
#define MCS_HPP

#include <string>
#include <vector>
#include <stdint.h>

namespace mcs {

struct Variable {
    std::string name;
    uint32_t ptr;
    uint32_t size;
    uint8_t type;
};

struct Directory {
    std::string name;
    uint32_t data_ptr;
    uint16_t var_num;
};

std::vector<Directory> get_directories();
std::vector<Variable> get_variables(const Directory& dir);

} // namespace mcs

#endif // MCS_HPP
