#include <cstdlib>

// Basic C++ runtime support for bare-metal SuperH with gint

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* p) noexcept {
    free(p);
}

void operator delete[](void* p) noexcept {
    free(p);
}

void operator delete(void* p, size_t) noexcept {
    free(p);
}

void operator delete[](void* p, size_t) noexcept {
    free(p);
}

// Exception handling stubs (since we use -fno-exceptions)
extern "C" {
    void __cxa_pure_virtual() { while (1); }
    void __cxa_deleted_virtual() { while (1); }

    // Some versions of GCC might still emit these symbols even with -fno-exceptions
    void __cxa_begin_catch() {}
    void __cxa_end_catch() {}
    void __cxa_rethrow() { while (1); }
}

namespace std {
    void __throw_bad_alloc() { while (1); }
    void __throw_bad_array_new_length() { while (1); }
    void __throw_length_error(char const*) { while (1); }
    void __throw_logic_error(char const*) { while (1); }
    void __throw_out_of_range(char const*) { while (1); }
    void __throw_out_of_range_fmt(char const*, ...) { while (1); }
}
