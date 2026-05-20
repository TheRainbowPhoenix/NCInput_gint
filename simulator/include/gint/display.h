#ifndef GINT_DISPLAY_H
#define GINT_DISPLAY_H

#include <stdint.h>

typedef uint16_t color_t;

#define C_WHITE 0xFFFF
#define C_BLACK 0x0000
#define C_NONE  0x0001
#define C_DARK  0x4210
#define C_LIGHT 0xCE79
#define C_BLUE  C_RGB(0, 0, 31)
#define C_RED   C_RGB(31, 0, 0)

#define C_RGB(r, g, b) (color_t)((((r) & 0x1f) << 11) | (((g) & 0x3f) << 5) | ((b) & 0x1f))

#define DTEXT_LEFT   0
#define DTEXT_CENTER 1
#define DTEXT_RIGHT  2
#define DTEXT_TOP    0
#define DTEXT_MIDDLE 4
#define DTEXT_BOTTOM 8

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SCREEN_W
#define SCREEN_W 320
#endif
#ifndef SCREEN_H
#define SCREEN_H 528
#endif

void drect(int x1, int y1, int x2, int y2, color_t color);
void drect_border(int x1, int y1, int x2, int y2, int fill_color, int width, int border_color);
void dline(int x1, int y1, int x2, int y2, color_t color);
void dhline(int y, color_t color);
void dvline(int x, color_t color);
void dtext(int x, int y, color_t color, const char *text);
void dtext_opt(int x, int y, color_t fg, int bg, int halign, int valign, const char *str, int size);
void dpoly(int const *x, int const *y, int n, int color, int border);
void dclear(color_t color);
void dupdate(void);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
struct dwindow {
    int x1, y1, x2, y2;
    dwindow() : x1(0), y1(0), x2(0), y2(0) {}
    dwindow(int _x1, int _y1, int _x2, int _y2) : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}
};
typedef struct dwindow dwindow_t;
#else
typedef struct {
    int x1, y1, x2, y2;
} dwindow_t;
static inline dwindow_t dwindow(int x1, int y1, int x2, int y2) {
    dwindow_t d = {x1, y1, x2, y2};
    return d;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

void dsize(const char *str, void const *font, int *w, int *h);
char const *drsize(const char *str, void const *font, int width, int *px);
void dwindow_set(dwindow_t window);

#ifdef __cplusplus
}
#endif

#endif
