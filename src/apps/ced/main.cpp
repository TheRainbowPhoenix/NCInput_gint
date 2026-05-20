#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <vector>
#include "editor.hpp"

#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
extern "C" void simulator_init();
#endif

int main() {
#if defined(SIMULATOR_NATIVE) || defined(SIMULATOR_WEB)
    simulator_init();
#endif

    ced::Editor editor;
    bool running = true;

    while (running) {
        editor.draw();

        cleareventflips();
        std::vector<key_event_t> events;
        key_event_t ev = pollevent();
        while (ev.type != KEYEV_NONE) {
            events.push_back(ev);
            ev = pollevent();
        }

        editor.update(events, running);
        sleep_ms(10);
    }

    return 0;
}
