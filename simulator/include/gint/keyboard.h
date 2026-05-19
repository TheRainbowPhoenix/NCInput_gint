#ifndef GINT_KEYBOARD_H
#define GINT_KEYBOARD_H

#include <stdint.h>

#define KEYEV_NONE 0
#define KEYEV_DOWN 1
#define KEYEV_UP   2
#define KEYEV_HOLD 3
#define KEYEV_TOUCH_DOWN 4
#define KEYEV_TOUCH_UP   5
#define KEYEV_TOUCH_DRAG 6

typedef struct {
    int type;
    int key;
    int x;
    int y;
} key_event_t;

#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  3
#define KEY_RIGHT 4
#define KEY_EXE   5
#define KEY_EXIT  6
#define KEY_DEL   7
#define KEY_MENU  8
#define KEY_F1    9

#ifdef __cplusplus
extern "C" {
#endif

void clearevents(void);
void cleareventflips(void);
key_event_t pollevent(void);
int keypressed(int key);
int getkey(void);

#ifdef __cplusplus
}
#endif

#endif
