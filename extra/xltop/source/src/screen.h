#ifndef _SCREEN_H_
#define _SCREEN_H_
#include <ev.h>

extern int screen_is_active;

int screen_init(void (*refresh_cb)(EV_P_ int LINES, int COLS), double interval);
void screen_start(EV_P);
void screen_stop(EV_P);
void screen_refresh(EV_P);
void screen_set_key_cb(void(*cb)(EV_P_ int c));

#define CP_BLACK 1
#define CP_RED 2
#define CP_GREEN 3
#define CP_YELLOW 4
#define CP_BLUE   5
#define CP_MAGENTA 5
#define CP_CYAN 7
#define CP_WHITE 8

#endif
