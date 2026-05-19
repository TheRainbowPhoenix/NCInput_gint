#include <SDL2/SDL.h>
#include "cinput.hpp"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <vector>
#include <queue>
#include <map>
#include <cstring>

// --- Global SDL State ---
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* screen_texture = nullptr;

// --- Keyboard State ---
static std::queue<key_event_t> event_queue;
static std::map<int, bool> pressed_keys;

// --- Helper: Color Conversion ---
static uint32_t rgb565_to_rgba8888(color_t c) {
    uint8_t r = ((c >> 11) & 0x1F) << 3;
    uint8_t g = ((c >> 5) & 0x3F) << 2;
    uint8_t b = (c & 0x1F) << 3;
    return (uint32_t)((r << 24) | (g << 16) | (b << 8) | 0xFF);
}

static void set_sdl_color(color_t c) {
    uint8_t r = ((c >> 11) & 0x1F) << 3;
    uint8_t g = ((c >> 5) & 0x3F) << 2;
    uint8_t b = (c & 0x1F) << 3;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
}

// --- Initialization ---
extern "C" void simulator_init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) return;
    window = SDL_CreateWindow("cinput Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
}

// --- Display API ---
void dclear(color_t color) {
    set_sdl_color(color);
    SDL_RenderClear(renderer);
}

void drect(int x1, int y1, int x2, int y2, color_t color) {
    set_sdl_color(color);
    SDL_Rect r = {x1, y1, x2 - x1, y2 - y1};
    SDL_RenderFillRect(renderer, &r);
}

void drect_border(int x1, int y1, int x2, int y2, int fill_color, int width, int border_color) {
    if (fill_color != C_NONE) drect(x1, y1, x2, y2, (color_t)fill_color);
    set_sdl_color((color_t)border_color);
    SDL_Rect r = {x1, y1, x2 - x1, y2 - y1};
    for (int i = 0; i < width; ++i) {
        SDL_RenderDrawRect(renderer, &r);
        r.x++; r.y++; r.w -= 2; r.h -= 2;
    }
}

void dline(int x1, int y1, int x2, int y2, color_t color) {
    set_sdl_color(color);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void dhline(int y, color_t color) {
    dline(0, y, SCREEN_W, y, color);
}

void dvline(int x, color_t color) {
    dline(x, 0, x, SCREEN_H, color);
}

void dtext(int x, int y, color_t color, const char *text) {
    if (!text) return;
    set_sdl_color(color);
    int cur_x = x;
    while (*text) {
        // Draw a 6x8 "block" for each char
        SDL_Rect r = {cur_x, y, 5, 7};
        SDL_RenderDrawRect(renderer, &r);
        cur_x += 8;
        text++;
    }
}

void dtext_opt(int x, int y, color_t fg, int bg, int halign, int valign, const char *str, int size) {
    if (!str) return;
    int len = strlen(str);
    int tw = len * 8;
    int th = 8;

    int tx = x;
    if (halign == DTEXT_CENTER) tx = x - tw / 2;
    else if (halign == DTEXT_RIGHT) tx = x - tw;

    int ty = y;
    if (valign == DTEXT_MIDDLE) ty = y - th / 2;
    else if (valign == DTEXT_BOTTOM) ty = y - th;

    if (bg != C_NONE) {
        drect(tx, ty, tx + tw, ty + th, (color_t)bg);
    }
    dtext(tx, ty, fg, str);
}

void dpoly(int const *x, int const *y, int n, int color, int border) {
    set_sdl_color((color_t)color);
    std::vector<SDL_Point> points(n + 1);
    for (int i = 0; i < n; ++i) points[i] = {x[i], y[i]};
    points[n] = {x[0], y[0]};
    SDL_RenderDrawLines(renderer, points.data(), n + 1);
}

void dupdate(void) {
    SDL_RenderPresent(renderer);

    // Pump events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) exit(0);

        key_event_t kev = {KEYEV_NONE, 0, 0, 0};
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            kev.type = KEYEV_TOUCH_DOWN;
            kev.x = e.button.x;
            kev.y = e.button.y;
            event_queue.push(kev);
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            kev.type = KEYEV_TOUCH_UP;
            kev.x = e.button.x;
            kev.y = e.button.y;
            event_queue.push(kev);
        } else if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
            kev.type = KEYEV_TOUCH_DRAG;
            kev.x = e.motion.x;
            kev.y = e.motion.y;
            event_queue.push(kev);
        } else if (e.type == SDL_KEYDOWN) {
            int key = 0;
            switch(e.key.keysym.sym) {
                case SDLK_UP: key = KEY_UP; break;
                case SDLK_DOWN: key = KEY_DOWN; break;
                case SDLK_LEFT: key = KEY_LEFT; break;
                case SDLK_RIGHT: key = KEY_RIGHT; break;
                case SDLK_RETURN: key = KEY_EXE; break;
                case SDLK_ESCAPE: key = KEY_EXIT; break;
                case SDLK_BACKSPACE: key = KEY_DEL; break;
            }
            if (key) {
                kev.type = KEYEV_DOWN;
                kev.key = key;
                event_queue.push(kev);
                pressed_keys[key] = true;
            }
        } else if (e.type == SDL_KEYUP) {
             int key = 0;
            switch(e.key.keysym.sym) {
                case SDLK_UP: key = KEY_UP; break;
                case SDLK_DOWN: key = KEY_DOWN; break;
                case SDLK_LEFT: key = KEY_LEFT; break;
                case SDLK_RIGHT: key = KEY_RIGHT; break;
                case SDLK_RETURN: key = KEY_EXE; break;
                case SDLK_ESCAPE: key = KEY_EXIT; break;
                case SDLK_BACKSPACE: key = KEY_DEL; break;
            }
            if (key) {
                pressed_keys[key] = false;
            }
        }
    }
}

// --- Keyboard API ---
void clearevents(void) {
    while(!event_queue.empty()) event_queue.pop();
}

void cleareventflips(void) {}

key_event_t pollevent(void) {
    if (event_queue.empty()) return {KEYEV_NONE, 0, 0, 0};
    key_event_t e = event_queue.front();
    event_queue.pop();
    return e;
}

int keypressed(int key) {
    return pressed_keys[key] ? 1 : 0;
}

int getkey(void) {
    while(true) {
        dupdate();
        key_event_t e = pollevent();
        if (e.type == KEYEV_DOWN) return e.key;
        SDL_Delay(10);
    }
}

// --- Timing API ---
uint32_t rtc_ticks(void) {
    return (uint32_t)(SDL_GetTicks() * 128 / 1000);
}

void sleep_ms(int ms) {
    SDL_Delay(ms);
}
