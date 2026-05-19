#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <string>
#include <vector>
#include "cinput.hpp"
#include "cdateinput.hpp"

using namespace cinput;

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif

    // Your app code here
    dclear(C_WHITE);
    dtext(20, 20, C_BLACK, "App Template");
    dupdate();
    getkey();

    return 0;
}
