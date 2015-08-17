#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "evx.h"
#include "screen.h"
#include "trace.h"

int screen_is_active;

static void (*screen_refresh_cb)(EV_P_ int LINES, int COLS);
static void (*screen_key_cb)(EV_P_ int);

static struct ev_timer refresh_timer_w;
static struct ev_io stdin_io_w;
static struct ev_signal sigint_w;
static struct ev_signal sigterm_w;
static struct ev_signal sigwinch_w;

static unsigned long bkgd_attr[2];

static void refresh_timer_cb(EV_P_ struct ev_timer *w, int revents);
static void stdin_io_cb(EV_P_ struct ev_io *w, int revents);
static void sigint_cb(EV_P_ ev_signal *w, int revents);
static void sigwinch_cb(EV_P_ ev_signal *w, int revents);

int screen_init(void (*refresh_cb)(EV_P_ int, int), double interval)
{
  screen_refresh_cb = refresh_cb;
  interval = 0.;
  ev_timer_init(&refresh_timer_w, &refresh_timer_cb, 0.001, interval);
  ev_io_init(&stdin_io_w, &stdin_io_cb, STDIN_FILENO, EV_READ);
  ev_signal_init(&sigint_w, &sigint_cb, SIGINT);
  ev_signal_init(&sigterm_w, &sigint_cb, SIGTERM);
  ev_signal_init(&sigwinch_w, &sigwinch_cb, SIGWINCH);

  return 0;
}

void screen_start(EV_P)
{
  short f[2], b[2];

  /* Begin curses magic. */
  if (initscr() == NULL)
    FATAL("cannot initialize screen: %m\n");

  cbreak();
  noecho();
  nonl();
  intrflush(stdscr, 0);
  keypad(stdscr, 1);
  nodelay(stdscr, 1);

  bkgd_attr[0] = getbkgd(stdscr);
  pair_content(bkgd_attr[0], &f[0], &b[0]);
  TRACE("bkgd[0] %lx, f %x, b %x\n", bkgd_attr[0],
        (unsigned int) f[0], (unsigned int) b[0]);

  if (!has_colors())
    ERROR("terminal has no color capabilities\n");

  use_default_colors();
  start_color();

  init_pair(CP_BLACK, -1, -1);
  init_pair(CP_RED, COLOR_RED, -1);
  init_pair(CP_YELLOW, COLOR_YELLOW, -1);
  init_pair(CP_GREEN, COLOR_GREEN, -1);
  /* init_pair(CP_BLUE, COLOR_BLUE, -1); */
  init_pair(CP_MAGENTA, COLOR_MAGENTA, -1);
  /* init_pair(CP_CYAN, COLOR_CYAN, -1); */
  /* init_pair(CP_WHITE, COLOR_WHITE, -1); */

  bkgd(COLOR_PAIR(1));
  bkgd_attr[1] = getbkgd(stdscr);
  pair_content(bkgd_attr[1], &f[1], &b[1]);
  TRACE("bkgd[1] %lx, f %x, b %x\n", bkgd_attr[1],
        (unsigned int) f[1], (unsigned int) b[1]);

  curs_set(0); /* Hide the cursor. */

  ev_timer_start(EV_A_ &refresh_timer_w);
  ev_io_start(EV_A_ &stdin_io_w);
  ev_signal_start(EV_A_ &sigint_w);
  ev_signal_start(EV_A_ &sigterm_w);
  ev_signal_start(EV_A_ &sigwinch_w);

  screen_is_active = 1;
}

void screen_refresh(EV_P)
{
  if (!ev_is_pending(&refresh_timer_w))
    ev_feed_event(EV_A_ &refresh_timer_w, EV_TIMER);
}

void screen_stop(EV_P)
{
  ev_timer_stop(EV_A_ &refresh_timer_w);
  endwin();

  screen_is_active = 0;
}

static void refresh_timer_cb(EV_P_ ev_timer *w, int revents)
{
  (*screen_refresh_cb)(EV_A_ LINES, COLS);
  ev_clear_pending(EV_A_ w);
  refresh();
}

void screen_set_key_cb(void(*cb)(EV_P_ int))
{
  screen_key_cb = cb;
}

static void stdin_io_cb(EV_P_ ev_io *w, int revents)
{
  int key = getch();
  if (key == ERR)
    return;

  TRACE("got `%c' from stdin\n", key);

  if (screen_key_cb != NULL) {
    (*screen_key_cb)(EV_A_ key);
    return;
  }
  /* TODO Remove switch. */
  switch (key) {
  case ' ':
  case '\n':
    screen_refresh(EV_A);
    break;
  case 'q':
    ev_break(EV_A_ EVBREAK_ALL);
    break;
  default:
    ERROR("unknown command `%c': try `h' for help\n", key);
    break;
  }
}

static void sigwinch_cb(EV_P_ ev_signal *w, int revents)
{
  TRACE("handling signal %d `%s'\n", w->signum, strsignal(w->signum));

  struct winsize ws;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0) {
    ERROR("cannot get window size: %m\n");
    return;
  }

  LINES = ws.ws_row;
  COLS = ws.ws_col;
  resizeterm(LINES, COLS);

  screen_refresh(EV_A);
}

static void sigint_cb(EV_P_ ev_signal *w, int revents)
{
  TRACE("handling signal %d `%s'\n", w->signum, strsignal(w->signum));

  ev_break(EV_A_ EVBREAK_ALL);
}
