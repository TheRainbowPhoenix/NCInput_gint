#include "mcs.hpp"
#include <cstring>

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
// Mock memory for simulator
static uint8_t mock_mcs[2048];
static bool mock_init = false;

static void init_mock() {
    if (mock_init) return;
    std::memset(mock_mcs, 0, sizeof(mock_mcs));
    // Base address is 0x8CF80100. We'll use offsets.
    // Directory 0: "MAT"
    std::memcpy(mock_mcs, "MAT", 3);
    *(uint32_t*)(mock_mcs + 8) = 0x8CF80500; // Fake pointer
    *(uint16_t*)(mock_mcs + 12) = 2; // 2 variables

    // Directory 1: "LIST"
    std::memcpy(mock_mcs + 16, "LIST", 4);
    *(uint32_t*)(mock_mcs + 16 + 8) = 0x8CF80600;
    *(uint16_t*)(mock_mcs + 16 + 12) = 1;

    // Variables for MAT (offset 0x400 from base)
    uint8_t* var_base = mock_mcs + 0x400;
    std::memcpy(var_base, "Mat A", 5);
    *(uint32_t*)(var_base + 8) = 0xDEADBEEF;
    *(uint32_t*)(var_base + 12) = 128;
    *(uint8_t*)(var_base + 16) = 0x01;

    std::memcpy(var_base + 20, "Mat B", 5);
    *(uint32_t*)(var_base + 28) = 0xCAFEBABE;
    *(uint32_t*)(var_base + 32) = 256;
    *(uint8_t*)(var_base + 36) = 0x01;

    mock_init = true;
}

static uint8_t read8(uint32_t addr) {
    init_mock();
    uint32_t offset = addr - 0x8CF80100;
    if (offset < sizeof(mock_mcs)) return mock_mcs[offset];
    return 0;
}
static uint16_t read16(uint32_t addr) {
    init_mock();
    uint32_t offset = addr - 0x8CF80100;
    if (offset + 1 < sizeof(mock_mcs)) return *(uint16_t*)(mock_mcs + offset);
    return 0;
}
static uint32_t read32(uint32_t addr) {
    init_mock();
    uint32_t offset = addr - 0x8CF80100;
    if (offset + 3 < sizeof(mock_mcs)) return *(uint32_t*)(mock_mcs + offset);
    return 0;
}
#else
// Direct access for hardware
static uint8_t read8(uint32_t addr) { return *(volatile uint8_t*)addr; }
static uint16_t read16(uint32_t addr) { return *(volatile uint16_t*)addr; }
static uint32_t read32(uint32_t addr) { return *(volatile uint32_t*)addr; }
#endif

namespace mcs {

static std::string get_string(uint32_t addr, int max_len) {
    std::string s;
    for (int i = 0; i < max_len; ++i) {
        uint8_t val = read8(addr + i);
        if (val == 0) break;
        if (val >= 32 && val < 127) s += (char)val;
        else s += '.';
    }
    return s;
}

std::vector<Directory> get_directories() {
    uint32_t DIR_BASE = 0x8CF80100;
    int MAX_DIRS = 0x87;
    std::vector<Directory> dirs;
    for (int i = 0; i < MAX_DIRS; ++i) {
        uint32_t addr = DIR_BASE + i * 16;
        std::string name = get_string(addr, 8);
        if (name.empty()) continue;

        Directory d;
        d.name = name;
        d.data_ptr = read32(addr + 8);
        d.var_num = read16(addr + 12);
        dirs.push_back(d);
    }
    return dirs;
}

std::vector<Variable> get_variables(const Directory& dir) {
    std::vector<Variable> vars;
    for (int j = 0; j < dir.var_num; ++j) {
        uint32_t addr = dir.data_ptr + j * 20;
        std::string name = get_string(addr, 8);
        if (name.empty()) continue;

        Variable v;
        v.name = name;
        v.ptr = read32(addr + 8);
        v.size = read32(addr + 12);
        v.type = read8(addr + 16);
        vars.push_back(v);
    }
    return vars;
}

} // namespace mcs
