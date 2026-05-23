#include "fm.hpp"
#include <gint/display.h>

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif

    fm::FileManager app;
    app.run();
    return 0;
}
